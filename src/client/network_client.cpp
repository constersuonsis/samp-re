#include "samp/client/network_client.h"

#include "PacketEnumerations.h"
#include "RakClientInterface.h"
#include "RakNetworkFactory.h"
#include "SHA1.h"
#include "samp/client/security_archive.h"
#include "samp/client/spawn.h"
#include "samp/game/memory.h"
#include "samp/util/logger.h"

#include <windows.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

namespace samp::client {
namespace {

enum class NetworkState {
  Connecting = 1,
  WaitingForResponse = 2,
  JoiningGame = 6
};

struct ServerGameSettings {
  std::array<bool, 11> flags{};
  std::array<std::uint32_t, 11> values{};
  std::uint16_t player_id = 0;
  std::array<std::uint8_t, 2> byte_values{};
  std::string hostname;
  std::array<char, 212> model_data{};
  std::uint32_t final_value = 0;
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
  std::string server_auth_key;
  ServerGameSettings server_game_settings;
  bool has_server_game_settings = false;
  NetworkState state = NetworkState::Connecting;
};

std::unique_ptr<NetworkClient> g_client;
int g_init_game_rpc_id = 139;
int g_connection_rejected_rpc_id = 130;
constexpr std::size_t kStuntBonusFlagIndex = 4;
constexpr std::size_t kGravityValueIndex = 4;
constexpr std::size_t kWeatherValueIndex = 1;

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

bool ReadAuthenticationKey(Packet &packet, unsigned offset, std::string &auth_key) {
  if (packet.length <= offset) {
    return false;
  }
  RakNet::BitStream stream(packet.data + offset, packet.length - offset, false);
  unsigned char message_id = 0;
  unsigned char key_length = 0;
  if (!stream.Read(message_id) || message_id != ID_AUTH_KEY || !stream.Read(key_length) ||
      stream.GetNumberOfUnreadBits() < key_length * 8) {
    return false;
  }
  std::array<char, 256> key_bytes{};
  if (!stream.Read(key_bytes.data(), key_length)) {
    return false;
  }
  auth_key.assign(key_bytes.data(), key_length);
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
    const size_t word_offset = index & ~static_cast<size_t>(3);
    const unsigned char value = digest[word_offset + (3 - (index & 3))];
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

bool SendAuthenticationKeyResponse(NetworkClient &client, const std::string &server_auth_key) {
  std::string hardware_id;
  if (!GenerateHardwareId(server_auth_key.c_str(), hardware_id) || hardware_id.size() > 255) {
    return false;
  }
  RakNet::BitStream payload;
  payload.Write(static_cast<unsigned char>(ID_AUTH_KEY));
  payload.Write(static_cast<unsigned char>(hardware_id.size()));
  payload.Write(hardware_id.data(), static_cast<int>(hardware_id.size()));
  return client.peer->Send(&payload, SYSTEM_PRIORITY, RELIABLE, 0);
}

bool ReadServerGameSettings(RakNet::BitStream &payload, ServerGameSettings &settings) {
  ServerGameSettings parsed;
  std::size_t flag_index = 0;
  std::size_t value_index = 0;
  auto read_flag = [&]() {
    return flag_index < parsed.flags.size() && payload.Read(parsed.flags[flag_index++]);
  };
  auto read_value = [&]() {
    return value_index < parsed.values.size() && payload.Read(parsed.values[value_index++]);
  };

  if (!read_flag() || !read_flag() || !read_flag() || !read_flag() || !read_value() ||
      !read_flag() || !read_value() || !read_flag() || !read_flag() || !read_flag() ||
      !read_value() || !payload.Read(parsed.player_id) || !read_flag() ||
      !read_value() || !payload.Read(parsed.byte_values[0]) || !payload.Read(parsed.byte_values[1]) ||
      !read_value() || !read_flag() || !read_value() || !read_flag()) {
    return false;
  }

  for (std::size_t index = 0; index < 5; ++index) {
    if (!read_value()) {
      return false;
    }
  }

  std::uint8_t hostname_length = 0;
  if (!payload.Read(hostname_length)) {
    return false;
  }
  const std::size_t required_bits =
      (static_cast<std::size_t>(hostname_length) + parsed.model_data.size() + sizeof(parsed.final_value)) * 8;
  if (static_cast<std::size_t>(payload.GetNumberOfUnreadBits()) < required_bits) {
    return false;
  }
  parsed.hostname.resize(hostname_length);
  if ((hostname_length != 0 && !payload.Read(parsed.hostname.data(), hostname_length)) ||
      !payload.Read(parsed.model_data.data(), static_cast<int>(parsed.model_data.size())) ||
      !payload.Read(parsed.final_value)) {
    return false;
  }
  settings = std::move(parsed);
  return true;
}

void ApplyServerGameOptions(const ServerGameSettings &settings) {
  samp::game::WriteByte(reinterpret_cast<void *>(0xA4A474),
                        settings.flags[kStuntBonusFlagIndex] ? 1 : 0);
  samp::game::WriteDword(reinterpret_cast<void *>(0x863984), settings.values[kGravityValueIndex]);
  ApplyServerWeather(settings.byte_values[kWeatherValueIndex]);
}

void HandleInitGameRpc(RPCParameters *parameters) {
  if (!parameters || !g_client || !parameters->input) {
    return;
  }
  const unsigned int payload_size = (parameters->numberOfBitsOfData + 7) / 8;
  RakNet::BitStream payload(parameters->input, payload_size, false);
  if (!ReadServerGameSettings(payload, g_client->server_game_settings)) {
    samp::util::WriteLogLine("network InitGame RPC rejected: truncated or invalid payload");
    return;
  }
  g_client->has_server_game_settings = true;
  ApplyServerGameOptions(g_client->server_game_settings);
  samp::util::WriteLogLine("network InitGame options applied; world initialization is pending");
}

void HandleConnectionRejectedRpc(RPCParameters *parameters) {
  if (!parameters || !g_client || !parameters->input || parameters->numberOfBitsOfData < 8) {
    return;
  }
  RakNet::BitStream payload(parameters->input, (parameters->numberOfBitsOfData + 7) / 8, false);
  unsigned char reason = 0;
  if (!payload.Read(reason)) {
    return;
  }
  switch (reason) {
    case 1:
      samp::util::WriteLogLine("network connection rejected: incorrect version");
      break;
    case 2:
      samp::util::WriteLogLine("network connection rejected: unacceptable nickname");
      samp::util::WriteLogLine("network nickname must be 3-20 alphanumeric characters");
      break;
    case 3:
      samp::util::WriteLogLine("network connection rejected: incompatible client version");
      break;
    case 4:
      samp::util::WriteLogLine("network connection rejected: no player slot available");
      break;
    default:
      samp::util::WriteLogNumber("network connection rejected: unknown reason", reason);
      break;
  }
  g_client->peer->Disconnect(500);
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
  client->peer->RegisterAsRemoteProcedureCall(&g_init_game_rpc_id, &HandleInitGameRpc);
  client->peer->RegisterAsRemoteProcedureCall(&g_connection_rejected_rpc_id,
                                               &HandleConnectionRejectedRpc);
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
        case ID_AUTH_KEY:
          if (ReadAuthenticationKey(*packet, offset, g_client->server_auth_key)) {
            samp::util::WriteLogLine(
                SendAuthenticationKeyResponse(*g_client, g_client->server_auth_key)
                    ? "network auth response sent"
                    : "network auth response generation or send failed");
          } else {
            samp::util::WriteLogLine("network auth key packet rejected");
          }
          break;
        case ID_CONNECTION_REQUEST_ACCEPTED:
          if (g_client->state != NetworkState::JoiningGame &&
              ReadConnectionAcceptance(*packet, offset, g_client->server_challenge)) {
            SendJoinRequest(*g_client);
          }
          break;
        case ID_CONNECTION_ATTEMPT_FAILED:
        case ID_NO_FREE_INCOMING_CONNECTIONS:
          g_client->state = NetworkState::Connecting;
          break;
        case ID_CONNECTION_LOST:
          peer.Disconnect(0);
          g_client->state = NetworkState::Connecting;
          break;
        case ID_DISCONNECTION_NOTIFICATION:
          peer.Disconnect(2000);
          break;
        case ID_CONNECTION_BANNED:
          samp::util::WriteLogLine("network connection rejected: banned");
          break;
        case ID_INVALID_PASSWORD:
          samp::util::WriteLogLine("network connection rejected: invalid password");
          peer.Disconnect(0);
          break;
        case ID_RSA_PUBLIC_KEY_MISMATCH:
          samp::util::WriteLogLine("network connection rejected: RSA key mismatch");
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
