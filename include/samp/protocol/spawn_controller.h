#pragma once

#include "samp/protocol/rpc_payloads.h"
#include "samp/protocol/samp_client.h"

namespace samp::protocol {

class SpawnController {
public:
    enum class Phase {

        Idle,

        AwaitingClass,

        ClassReady,

        AwaitingDecision,
    };

    explicit SpawnController(SampClient& client);

    bool SelectClass(std::int32_t classIndex);

    bool RequestSpawn();

    Phase GetPhase() const { return phase_; }

    bool HasSpawnInfo() const { return hasSpawnInfo_; }

    const SpawnInfo& GetSpawnInfo() const { return spawnInfo_; }

    SpawnDecision GetLastDecision() const { return lastDecision_; }

private:
    void BindResponses();

    SampClient& client_;
    Phase phase_ = Phase::Idle;
    SpawnInfo spawnInfo_{};
    SpawnDecision lastDecision_ = SpawnDecision::Refused;
    bool hasSpawnInfo_ = false;
};

}
