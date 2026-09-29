#pragma once

#include <cstdint>

namespace samp::client {

bool CreateNetworkClient(const char *host, unsigned short port, const char *name,
                         const char *password);
bool HasNetworkClient();
int NetworkClientState();
void TryConnectNetworkClient();
bool RequestServerClassSelection(std::uint32_t class_id);
bool RequestServerSpawn();
void PumpNetworkClient();
void DestroyNetworkClient();

}
