#include "samp/protocol/rpc_ids.h"
#include "samp/network/bit_stream.h"

#include <algorithm>
#include <cstdio>
#include <vector>

namespace {

int g_failures = 0;

void Check(bool condition, const char* what) {
    if (!condition) {
        std::printf("FAIL: %s\n", what);
        ++g_failures;
    }
}

std::vector<samp::protocol::RpcId> IncomingIds() {
    using samp::protocol::RpcId;
    return {RpcId::SetPlayerPosition,     RpcId::SetPlayerHealth,
            RpcId::SetCameraMode,         RpcId::SetPlayerAnimationSpeed,
            RpcId::GetCameraSpeed,        RpcId::ProcessAnimation,
            RpcId::SetPlayerArmour,       RpcId::SetPlayerColour,
            RpcId::SetGravity,            RpcId::SetVehicleHealth,
            RpcId::SetObjectSpeed,        RpcId::SetWidescreen,
            RpcId::ClearObjectMovement,   RpcId::SetObjectCurrentRotation,
            RpcId::SetObjectCollision,    RpcId::MoveObject,
            RpcId::SetTextDrawString,
            RpcId::ShowTextDraw,          RpcId::HideTextDraw,
            RpcId::StopObject,
            RpcId::SetPlayerInterior,
            RpcId::SetPlayerCameraPosition, RpcId::SetPlayerCameraLookAt,
            RpcId::SetVehiclePosition,    RpcId::SetVehicleZAngle,
            RpcId::TogglePlayerControllable, RpcId::PlaySound,
            RpcId::SetPlayerWorldBounds,  RpcId::CreateObject,
            RpcId::DestroyObject,         RpcId::RemoveBuildingForPlayer,
            RpcId::DeathMessage,          RpcId::SelectTextDraw,
            RpcId::SetObjectMaterial,     RpcId::EmptyPacket,
            RpcId::ApplyObjectTargetRotation, RpcId::ApplyObjectMovement,
            RpcId::SetPlayerMapIcon,
            RpcId::RemoveVehicleComponent, RpcId::LinkVehicleToInterior,
            RpcId::SetVehicleNumberPlate, RpcId::AttachTrailerToVehicle,
            RpcId::DetachTrailerFromVehicle,
            RpcId::SetPlayerPositionFindZ, RpcId::PutPlayerInVehicle,
            RpcId::RemovePlayerFromVehicle, RpcId::SetVehicleParamsForPlayer,
            RpcId::DisableVehicleCollisions,
            RpcId::SetPlayerName,         RpcId::SetSpawnInfo,
            RpcId::SetPlayerTeam,
            RpcId::DisplayGameText,       RpcId::AttachObjectToPlayer,
            RpcId::SetPlayerArmedWeapon,
            RpcId::SetPlayerWantedLevel,
            RpcId::GivePlayerWeapon,      RpcId::SetPlayerAmmo,
            RpcId::RemovePlayerMapIcon,   RpcId::TogglePlayerSpectating,
            RpcId::PlayerSpectatePlayer,  RpcId::PlayerSpectateVehicle,
            RpcId::SetVehicleParamsEx,    RpcId::EnterVehicle,
            RpcId::EnterEditObject,       RpcId::CancelEdit,
            RpcId::SetPlayerTime,         RpcId::ToggleClock,
            RpcId::WorldPlayerAdd,        RpcId::SetPlayerSkillLevel,
            RpcId::SetPlayerShopName,     RpcId::SetPlayerDrunkLevel,
            RpcId::PlayAudioStream,       RpcId::Create3DTextLabel,
            RpcId::Remove3DTextLabel,
            RpcId::DisableCheckpoint,
            RpcId::SetRaceCheckpoint,     RpcId::DisableRaceCheckpoint,
            RpcId::GameModeRestart,       RpcId::StopAudioStream,
            RpcId::DownloadProgress,      RpcId::ForceClassSelection,
            RpcId::SetCameraBehindPlayer,
            RpcId::ChatBubble,            RpcId::SomeUpdate,
            RpcId::ShowDialog,            RpcId::DestroyPickup,
            RpcId::ClientMessage,         RpcId::SetWorldTime,
            RpcId::CreatePickup,          RpcId::ScmEvent,
            RpcId::Chat,                  RpcId::ServerNetStats,
            RpcId::ClientCheck,           RpcId::VehicleDamageStatus,
            RpcId::SetCheckpoint,         RpcId::SetPlayerAttachedObject,
            RpcId::EditAttachedObject,
            RpcId::EditObject,            RpcId::RequestClass,
            RpcId::RequestSpawn,          RpcId::ConnectionRejected,
            RpcId::ServerJoin,            RpcId::ServerQuit,
            RpcId::InitGame,              RpcId::QueueDownload,
            RpcId::SetWeather,            RpcId::ExitVehicle,
            RpcId::UpdateScoresPingsIPs,  RpcId::WorldPlayerRemove,
            RpcId::WorldVehicleAdd,       RpcId::WorldVehicleRemove,
            RpcId::WorldPlayerDeath,      RpcId::ToggleCameraTarget,
            RpcId::ShowActor,             RpcId::HideActor,
            RpcId::ApplyActorAnimation,   RpcId::ClearActorAnimation,
            RpcId::ClearAnimations,       RpcId::SetPlayerSpecialAction,
            RpcId::SetVehicleDoors,       RpcId::EnableStuntBonus,
            RpcId::SetActorFacingAngle,
            RpcId::SetActorPosition,      RpcId::SetActorHealth,
            RpcId::PlayAnimation,         RpcId::SetPlayerAnimationByIndex,
            RpcId::SetPlayerAnimationByIndexAlt, RpcId::SetVehicleControllable,
            RpcId::SpawnPlayerFull,       RpcId::SetPlayerSkin};
}

void TestRegisteredIdsAreUnique() {
    std::vector<std::uint8_t> values;
    for (samp::protocol::RpcId id : IncomingIds()) {
        values.push_back(samp::protocol::ToByte(id));
    }

    std::sort(values.begin(), values.end());
    const auto duplicate = std::adjacent_find(values.begin(), values.end());
    Check(duplicate == values.end(), "no two handlers share an id");
    Check(values.size() == 130, "every registered handler is accounted for");
}

void TestIdsFitOneByte() {

    for (samp::protocol::RpcId id : IncomingIds()) {
        samp::net::BitStream stream;
        stream.Write(samp::protocol::ToByte(id));

        std::uint8_t readBack = 0;
        if (!stream.Read(readBack) || readBack != samp::protocol::ToByte(id)) {
            Check(false, "rpc id survives a byte-sized round-trip");
            return;
        }
    }
}

void TestEmptyBodiedIdsAreMarked() {
    using samp::protocol::HasEmptyBody;
    using samp::protocol::RpcId;

    Check(HasEmptyBody(RpcId::GameModeRestart), "a game mode restart has no body");
    Check(HasEmptyBody(RpcId::DisableCheckpoint), "disabling a checkpoint has no body");
    Check(HasEmptyBody(RpcId::StopAudioStream), "stopping the stream has no body");
    Check(HasEmptyBody(RpcId::SetCameraBehindPlayer), "resetting the camera has no body");

    Check(!HasEmptyBody(RpcId::SetCheckpoint), "setting a checkpoint does carry fields");
    Check(!HasEmptyBody(RpcId::PlayAudioStream), "starting a stream does carry fields");
    Check(!HasEmptyBody(RpcId::SetPlayerCameraPosition), "moving the camera carries fields");
}

void TestPlayerAndVehicleDamageAreDistinct() {

    Check(samp::protocol::ToByte(samp::protocol::RpcId::GiveTakeDamage) !=
              samp::protocol::ToByte(samp::protocol::RpcId::VehicleDamageStatus),
          "player damage and vehicle damage use different ids");
}

}

int main() {
    TestRegisteredIdsAreUnique();
    TestIdsFitOneByte();
    TestEmptyBodiedIdsAreMarked();
    TestPlayerAndVehicleDamageAreDistinct();

    if (g_failures == 0) {
        std::printf("All RPC id tests passed.\n");
        return 0;
    }

    std::printf("%d check(s) failed.\n", g_failures);
    return 1;
}
