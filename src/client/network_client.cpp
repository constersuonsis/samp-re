#include "samp/client/network_client.h"

#include "PacketEnumerations.h"
#include "RakClientInterface.h"
#include "RakNetworkFactory.h"

#include <windows.h>

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
  NetworkState state = NetworkState::Connecting;
};

std::unique_ptr<NetworkClient> g_client;

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
  bool started = g_client->peer->Connect(g_client->host.c_str(), g_client->port, 0, 0, 2,
                                         nullptr);
  g_client->last_attempt = now;
  g_client->state = started ? NetworkState::WaitingForResponse : NetworkState::Connecting;
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
          g_client->state = NetworkState::JoiningGame;
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
