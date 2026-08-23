#include "samp/network/raknet_client.h"

#include "BitStream.h"
#include "PacketEnumerations.h"
#include "RakClient.h"
#include "RakNetworkFactory.h"

namespace samp::net {
namespace {

static_assert(static_cast<int>(Priority::System) == SYSTEM_PRIORITY, "priority numbering");
static_assert(static_cast<int>(Priority::High) == HIGH_PRIORITY, "priority numbering");
static_assert(static_cast<int>(Priority::Medium) == MEDIUM_PRIORITY, "priority numbering");
static_assert(static_cast<int>(Priority::Low) == LOW_PRIORITY, "priority numbering");

static_assert(static_cast<int>(Reliability::Unreliable) == UNRELIABLE, "reliability numbering");
static_assert(static_cast<int>(Reliability::UnreliableSequenced) == UNRELIABLE_SEQUENCED,
              "reliability numbering");
static_assert(static_cast<int>(Reliability::Reliable) == RELIABLE, "reliability numbering");
static_assert(static_cast<int>(Reliability::ReliableOrdered) == RELIABLE_ORDERED,
              "reliability numbering");

PacketPriority ToRakNetPriority(Priority priority) {
    return static_cast<PacketPriority>(priority);
}

PacketReliability ToRakNetReliability(Reliability reliability) {
    return static_cast<PacketReliability>(reliability);
}

}

class RakNetClient::Peer {
public:
    Peer() : client_(RakNetworkFactory::GetRakClientInterface()) {
        client_->SetRpcHandler(&Peer::OnRemoteCall, this);
    }

    ~Peer() { RakNetworkFactory::DestroyRakClientInterface(client_); }

    Peer(const Peer&) = delete;
    Peer& operator=(const Peer&) = delete;

    RakClientInterface& Client() { return *client_; }
    const RakClientInterface& Client() const { return *client_; }

    void SetHandler(ConnectionHandler* handler) { handler_ = handler; }
    ConnectionHandler* Handler() const { return handler_; }

private:
    static void OnRemoteCall(unsigned char rpcId, RakNet::BitStream* payload,
                             RakPeerInterface* , void* context) {
        auto* self = static_cast<Peer*>(context);
        if (self == nullptr || self->handler_ == nullptr || payload == nullptr) {
            return;
        }

        const BitStream incoming(payload->GetData(), payload->GetNumberOfBytesUsed(), true);
        self->handler_->OnRpc(rpcId, incoming);
    }

    RakClientInterface* client_;
    ConnectionHandler* handler_ = nullptr;
};

RakNetClient::RakNetClient() : peer_(std::make_unique<Peer>()) {}

RakNetClient::~RakNetClient() = default;

bool RakNetClient::Connect(const char* host, std::uint16_t serverPort, std::uint16_t clientPort,
                           const char* password) {
    if (password != nullptr) {
        peer_->Client().SetPassword(password);
    }
    return peer_->Client().Connect(host, serverPort, clientPort, 0, 30, nullptr);
}

void RakNetClient::SetHandler(ConnectionHandler* handler) {
    peer_->SetHandler(handler);
}

void RakNetClient::Pump() {
    constexpr std::uint8_t kFirstGamePacketId = 200;
    constexpr unsigned int kTimestampSize = sizeof(unsigned char) + sizeof(RakNetTime);

    RakClientInterface& client = peer_->Client();

    while (Packet* packet = client.Receive()) {
        ConnectionHandler* handler = peer_->Handler();
        if (handler != nullptr) {
            const unsigned char* data = packet->data;
            unsigned int length = packet->length;

            if (data[0] == ID_TIMESTAMP && length > kTimestampSize) {
                data += kTimestampSize;
                length -= kTimestampSize;
            }

            switch (data[0]) {
                case ID_CONNECTION_REQUEST_ACCEPTED:
                    handler->OnConnected();
                    break;
                case ID_DISCONNECTION_NOTIFICATION:
                case ID_CONNECTION_LOST:
                case ID_NO_FREE_INCOMING_CONNECTIONS:
                case ID_CONNECTION_ATTEMPT_FAILED:
                case ID_CONNECTION_BANNED:
                case ID_INVALID_PASSWORD:
                    handler->OnDisconnected();
                    break;
                default:
                    if (data[0] >= kFirstGamePacketId) {
                        const BitStream incoming(data, length, false);
                        handler->OnPacket(incoming);
                    }
                    break;
            }
        }
        client.DeallocatePacket(packet);
    }
}

bool RakNetClient::Send(const BitStream& packet, const DeliveryOptions& options) {
    return peer_->Client().Send(reinterpret_cast<const char*>(packet.GetData()),
                                static_cast<int>(packet.GetNumberOfBytesUsed()),
                                ToRakNetPriority(options.priority),
                                ToRakNetReliability(options.reliability),
                                static_cast<char>(options.orderingChannel));
}

bool RakNetClient::SendRpc(std::uint8_t rpcId, const BitStream& payload,
                           const DeliveryOptions& options) {
    int callId = rpcId;
    RakNet::BitStream parameters;
    parameters.Write(reinterpret_cast<const char*>(payload.GetData()),
                     static_cast<int>(payload.GetNumberOfBytesUsed()));

    return peer_->Client().RPC(&callId, &parameters, ToRakNetPriority(options.priority),
                               ToRakNetReliability(options.reliability),
                               static_cast<char>(options.orderingChannel), false,
                               UNASSIGNED_NETWORK_ID, nullptr);
}

void RakNetClient::Disconnect(std::uint32_t lingerMs) {
    peer_->Client().Disconnect(lingerMs, 0);
}

bool RakNetClient::IsConnected() const {
    return peer_->Client().IsConnected();
}

}
