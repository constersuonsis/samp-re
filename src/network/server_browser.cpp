#include "samp/network/server_browser.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <vector>

namespace samp::browser {

namespace {

#pragma pack(push, 1)
struct QueryHeader {
    char magic[4];
    std::uint16_t port;
    char opcode;
};
#pragma pack(pop)

void WriteBigEndianU16(std::vector<char>& out, std::uint16_t value) {
    out.push_back(static_cast<char>(value >> 8));
    out.push_back(static_cast<char>(value & 0xFF));
}

std::uint16_t ReadBigEndianU16(const char* data) {
    return static_cast<std::uint16_t>(static_cast<std::uint8_t>(data[0]) << 8 |
                                      static_cast<std::uint8_t>(data[1]));
}

std::uint16_t ReadLittleEndianU16(const char* data) {
    return static_cast<std::uint16_t>(static_cast<std::uint8_t>(data[0]) |
                                      static_cast<std::uint8_t>(data[1]) << 8);
}

bool ReadString(const char* data, std::size_t size, std::size_t& offset, std::string& value) {
    if (offset + 2 > size) {
        return false;
    }
    const std::uint16_t length = ReadBigEndianU16(data + offset);
    offset += 2;
    if (offset + length > size) {
        return false;
    }
    value.assign(data + offset, length);
    offset += length;
    return true;
}

constexpr char kMagic[4] = {'S', 'A', 'M', 'P'};
constexpr char kInfoOpcode = 'i';

}

std::optional<ServerInfo> QueryInfo(const char* host, std::uint16_t serverPort,
                                    unsigned timeoutMs) {
    SOCKET socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socketHandle == INVALID_SOCKET) {
        return std::nullopt;
    }

    sockaddr_in destination{};
    destination.sin_family = AF_INET;
    destination.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, host, &destination.sin_addr) != 1) {
        closesocket(socketHandle);
        return std::nullopt;
    }

    std::vector<char> request;
    request.insert(request.end(), kMagic, kMagic + 4);
    WriteBigEndianU16(request, serverPort);
    request.push_back(kInfoOpcode);

    if (sendto(socketHandle, request.data(), static_cast<int>(request.size()), 0,
               reinterpret_cast<const sockaddr*>(&destination), sizeof(destination)) ==
        SOCKET_ERROR) {
        std::printf("[dbg] sendto failed %d\n", WSAGetLastError());
        closesocket(socketHandle);
        return std::nullopt;
    }

    timeval wait{};
    wait.tv_sec = timeoutMs / 1000;
    wait.tv_usec = (timeoutMs % 1000) * 1000;
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(socketHandle, &readSet);
    if (select(0, &readSet, nullptr, nullptr, &wait) != 1) {
        closesocket(socketHandle);
        return std::nullopt;
    }

    std::vector<char> reply(1024);
    sockaddr_in from{};
    int fromLength = sizeof(from);
    const int received = recvfrom(socketHandle, reply.data(), static_cast<int>(reply.size()), 0,
                                  reinterpret_cast<sockaddr*>(&from), &fromLength);
    closesocket(socketHandle);
    if (received < 12) {

        return std::nullopt;
    }

    std::size_t offset = 7;
    ServerInfo info;
    info.passworded = reply[offset] != 0;
    offset += 1;
    info.players = ReadLittleEndianU16(reply.data() + offset);
    offset += 2;
    info.maxPlayers = ReadLittleEndianU16(reply.data() + offset);
    offset += 2;

    if (!ReadString(reply.data(), static_cast<std::size_t>(received), offset,
                    info.hostname) ||
        !ReadString(reply.data(), static_cast<std::size_t>(received), offset,
                    info.gameMode) ||
        !ReadString(reply.data(), static_cast<std::size_t>(received), offset,
                    info.language)) {
        return std::nullopt;
    }

    return info;
}

}
