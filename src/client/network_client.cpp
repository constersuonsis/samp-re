#include "samp/client/network_client.h"

#include "PacketEnumerations.h"
#include "RakClientInterface.h"
#include "RakNetworkFactory.h"
#include "SHA1.h"

#include <windows.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

namespace samp::client {
namespace {

enum class NetworkState {
  Connecting = 1,
  WaitingForResponse = 2,
  JoiningGame = 6
};

struct ClientDeleter {
  void operator()(RakClientInterface *client) const {
    if (client) {
      client->Disconnect(0);
      RakNetworkFactory::DestroyRakClientInterface(client);
    }
  }
};

struct NetworkClient {
  std::unique_ptr<RakClientInterface, ClientDeleter> peer;
  std::string host;
  std::string name;
  std::string password;
  unsigned short port = 0;
  DWORD last_attempt = 0;
  std::uint32_t server_challenge = 0;
  NetworkState state = NetworkState::Connecting;
};

std::unique_ptr<NetworkClient> g_client;

bool ReadConnectionAcceptance(Packet &packet, unsigned offset,
                              std::uint32_t &server_challenge) {
  if (packet.length <= offset) {
    return false;
  }
  RakNet::BitStream stream(packet.data + offset, packet.length - offset, false);
  unsigned char message_id = 0;
  unsigned int remote_address = 0;
  unsigned short remote_port = 0;
  unsigned short player_index = 0;
  if (!stream.Read(message_id) || message_id != ID_CONNECTION_REQUEST_ACCEPTED ||
      !stream.Read(remote_address) || !stream.Read(remote_port) || !stream.Read(player_index) ||
      !stream.Read(server_challenge)) {
    return false;
  }
  (void)remote_address;
  (void)remote_port;
  (void)player_index;
  server_challenge ^= 0xFD9;
  return true;
}

bool GenerateJoinKey(char *key, size_t capacity) {
  auto *path = reinterpret_cast<unsigned char *>(0xC9236C);
  const size_t path_length = strnlen_s(reinterpret_cast<const char *>(path), 256);
  if (path_length == 256 || capacity < 2) {
    return false;
  }

  CSHA1 sha1;
  sha1.Update(path, static_cast<unsigned int>(path_length));
  sha1.Final();
  const unsigned char *digest = sha1.GetHash();
  std::array<std::uint32_t, 6> limbs{};
  for (size_t index = 0; index < SHA1_LENGTH; ++index) {
    const unsigned char value = digest[index];
    const unsigned char first = value & 3;
    const unsigned char second = (value >> 2) & 3;
    const unsigned char third = (value >> 4) & 3;
    const unsigned char fourth = value >> 6;
    const unsigned char low_pair = first <= second ? first | (second << 2)
                                                   : second | (first << 2);
    const unsigned char high_pair = third <= fourth ? third | (fourth << 2)
                                                     : fourth | (third << 2);
    const unsigned char encoded = low_pair | (high_pair << 4);
    std::uint64_t carry = encoded;
    for (std::uint32_t &limb : limbs) {
      const std::uint64_t value_with_carry =
          (static_cast<std::uint64_t>(limb) << 8) | carry;
      limb = static_cast<std::uint32_t>(value_with_carry);
      carry = value_with_carry >> 32;
    }
    if (carry != 0) {
      return false;
    }
  }

  std::uint64_t carry = 0;
  for (std::uint32_t &limb : limbs) {
    const std::uint64_t product = static_cast<std::uint64_t>(limb) * 1001 + carry;
    limb = static_cast<std::uint32_t>(product);
    carry = product >> 32;
  }
  if (carry != 0) {
    return false;
  }

  static constexpr char digits[] = "0123456789ABCDEF";
  size_t output_length = 0;
  bool started = false;
  for (size_t limb_index = limbs.size(); limb_index-- > 0;) {
    for (int shift = 28; shift >= 0; shift -= 4) {
      const unsigned char digit = static_cast<unsigned char>((limbs[limb_index] >> shift) & 0xF);
      if (!started && digit == 0) {
        continue;
      }
      if (output_length + 1 >= capacity) {
        return false;
      }
      key[output_length++] = digits[digit];
      started = true;
    }
  }
  if (!started) {
    key[output_length++] = '0';
  }
  key[output_length] = 0;
  return true;
}

bool SendJoinRequest(NetworkClient &client) {
  char key[64] = {};
  if (!GenerateJoinKey(key, sizeof(key)) || client.name.size() > 255) {
    return false;
  }

  constexpr char version[] = "0.3.7-R3";
  const auto name_length = static_cast<unsigned char>(client.name.size());
  const auto key_length = static_cast<unsigned char>(std::strlen(key));
  const auto version_length = static_cast<unsigned char>(sizeof(version) - 1);
  RakNet::BitStream payload;
  payload.Write(static_cast<std::uint32_t>(0xFD9));
  payload.Write(static_cast<unsigned char>(1));
  payload.Write(name_length);
  payload.Write(client.name.data(), static_cast<int>(client.name.size()));
  payload.Write(client.server_challenge);
  payload.Write(key_length);
  payload.Write(key, key_length);
  payload.Write(version_length);
  payload.Write(version, version_length);
  payload.Write(client.server_challenge);

  int rpc_id = 25;
  client.state = NetworkState::JoiningGame;
  return client.peer->RPC(&rpc_id, &payload, HIGH_PRIORITY, RELIABLE, 0, false,
                          UNASSIGNED_NETWORK_ID, nullptr);
}

}

bool CreateNetworkClient(const char *host, unsigned short port, const char *name,
                         const char *password) {
  if (g_client) {
    return true;
  }
  if (!host || !host[0] || !name || !name[0] || !port) {
    return false;
  }
  auto client = std::make_unique<NetworkClient>();
  client->peer.reset(RakNetworkFactory::GetRakClientInterface());
  if (!client->peer) {
    return false;
  }
  client->host = host;
  client->name = name;
  client->password = password ? password : "";
  client->port = port;
  client->last_attempt = ::GetTickCount();
  client->peer->SetPassword(client->password.c_str());
  g_client = std::move(client);
  return true;
}

bool HasNetworkClient() {
  return g_client != nullptr;
}

int NetworkClientState() {
  return g_client ? static_cast<int>(g_client->state) : 0;
}

void TryConnectNetworkClient() {
  if (!g_client || g_client->state != NetworkState::Connecting) {
    return;
  }
  DWORD now = ::GetTickCount();
  if (now - g_client->last_attempt <= 3000) {
    return;
  }
  g_client->peer->Connect(g_client->host.c_str(), g_client->port, 0, 0, 2, nullptr);
  g_client->last_attempt = now;
  g_client->state = NetworkState::WaitingForResponse;
}

void PumpNetworkClient() {
  if (!g_client) {
    return;
  }
  RakClientInterface &peer = *g_client->peer;
  while (Packet *packet = peer.Receive()) {
    unsigned offset = 0;
    if (packet->length > sizeof(RakNetTime) + 1 && packet->data[0] == ID_TIMESTAMP) {
      offset = sizeof(RakNetTime) + 1;
    }
    if (packet->length > offset) {
      switch (packet->data[offset]) {
        case ID_CONNECTION_REQUEST_ACCEPTED:
          if (g_client->state != NetworkState::JoiningGame &&
              ReadConnectionAcceptance(*packet, offset, g_client->server_challenge)) {
            SendJoinRequest(*g_client);
          }
          break;
        case ID_DISCONNECTION_NOTIFICATION:
        case ID_CONNECTION_LOST:
        case ID_NO_FREE_INCOMING_CONNECTIONS:
        case ID_CONNECTION_ATTEMPT_FAILED:
        case ID_CONNECTION_BANNED:
        case ID_INVALID_PASSWORD:
          g_client->state = NetworkState::Connecting;
          g_client->last_attempt = ::GetTickCount();
          break;
        default:
          break;
      }
    }
    peer.DeallocatePacket(packet);
  }
}

void DestroyNetworkClient() {
  g_client.reset();
}

}
