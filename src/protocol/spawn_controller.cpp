#include "samp/protocol/spawn_controller.h"

namespace samp::protocol {

SpawnController::SpawnController(SampClient& client) : client_(client) {
    BindResponses();
}

void SpawnController::BindResponses() {
    RpcDispatcher& dispatcher = client_.Dispatcher();

    dispatcher.On<RequestClassResponse>(
        RpcId::RequestClass, &ReadRequestClassResponse,
        [this](const RequestClassResponse& response) {
            if (response.accepted != 0) {
                spawnInfo_ = response.spawnInfo;
                hasSpawnInfo_ = true;
                phase_ = Phase::ClassReady;
            } else {

                phase_ = Phase::Idle;
            }
        });

    dispatcher.On<RequestSpawnResponse>(
        RpcId::RequestSpawn, &ReadRequestSpawnResponse,
        [this](const RequestSpawnResponse& response) {
            lastDecision_ = static_cast<SpawnDecision>(response.decision);

            phase_ = Phase::Idle;
        });
}

bool SpawnController::SelectClass(std::int32_t classIndex) {
    if (!client_.SendClassSelection(classIndex)) {
        return false;
    }
    phase_ = Phase::AwaitingClass;
    return true;
}

bool SpawnController::RequestSpawn() {
    if (phase_ != Phase::ClassReady || !client_.SendSpawnRequest()) {
        return false;
    }
    phase_ = Phase::AwaitingDecision;
    return true;
}

}
