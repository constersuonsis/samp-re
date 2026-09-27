#pragma once

namespace samp::client {

bool CreateNetworkClient(const char *host, unsigned short port, const char *name,
                         const char *password);
bool HasNetworkClient();
int NetworkClientState();
void TryConnectNetworkClient();
void PumpNetworkClient();
void DestroyNetworkClient();

}
