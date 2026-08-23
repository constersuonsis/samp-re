#pragma once

#include <cstdint>

namespace samp::protocol {

enum class RpcId : std::uint8_t {

    SetPlayerName = 11,
    SetPlayerPosition = 12,
    SetPlayerPositionFindZ = 13,
    SetPlayerHealth = 14,
    TogglePlayerControllable = 15,
    PlaySound = 16,
    SetPlayerWorldBounds = 17,
    SetCameraMode = 18,
    SetPlayerAnimationSpeed = 19,
    GetCameraSpeed = 20,
    ProcessAnimation = 21,
    GivePlayerWeapon = 22,
    SetVehicleParamsEx = 24,
    EnterVehicle = 26,
    EnterEditObject = 27,
    CancelEdit = 28,
    SetPlayerTime = 29,
    ToggleClock = 30,
    WorldPlayerAdd = 32,
    SetPlayerShopName = 33,
    SetPlayerSkillLevel = 34,
    SetPlayerDrunkLevel = 35,
    Create3DTextLabel = 36,
    DisableCheckpoint = 37,
    SetRaceCheckpoint = 38,
    DisableRaceCheckpoint = 39,
    GameModeRestart = 40,
    StopAudioStream = 42,
    PlayAudioStream = 41,
    RemoveBuildingForPlayer = 43,
    CreateObject = 44,
    SetObjectSpeed = 46,
    DestroyObject = 47,
    DownloadProgress = 48,
    DeathMessage = 55,
    SetPlayerMapIcon = 56,
    RemoveVehicleComponent = 57,

    Remove3DTextLabel = 58,
    ChatBubble = 59,
    SomeUpdate = 60,
    ShowDialog = 61,
    DestroyPickup = 63,
    EmptyPacket = 64,
    SetPlayerArmour = 66,
    SetPlayerArmedWeapon = 67,
    LinkVehicleToInterior = 65,
    SetSpawnInfo = 68,
    SetPlayerTeam = 69,
    PutPlayerInVehicle = 70,
    RemovePlayerFromVehicle = 71,
    DisplayGameText = 73,
    ForceClassSelection = 74,
    AttachObjectToPlayer = 75,
    SelectTextDraw = 83,
    SetObjectMaterial = 84,
    ApplyObjectTargetRotation = 85,
    ClearAnimations = 87,
    SetPlayerSpecialAction = 89,
    SetVehicleDoors = 98,
    SetPlayerColour = 72,
    MoveObject = 99,
    SetTextDrawString = 105,
    ClearObjectMovement = 120,
    SetObjectCurrentRotation = 121,
    StopObject = 122,
    SetVehicleNumberPlate = 123,
    TogglePlayerSpectating = 124,
    PlayerSpectatePlayer = 126,
    PlayerSpectateVehicle = 127,
    SetPlayerWantedLevel = 133,
    ShowTextDraw = 134,
    HideTextDraw = 135,
    RemovePlayerMapIcon = 144,
    SetPlayerAmmo = 145,
    ClientMessage = 93,
    SetWorldTime = 94,
    CreatePickup = 95,
    ScmEvent = 96,
    Chat = 101,
    ServerNetStats = 102,
    ClientCheck = 103,
    EnableStuntBonus = 104,

    VehicleDamageStatus = 106,
    SetCheckpoint = 107,
    ApplyObjectMovement = 108,
    SetWidescreen = 111,
    SetPlayerAttachedObject = 113,
    EditAttachedObject = 116,
    EditObject = 117,
    RequestClass = 128,
    RequestSpawn = 129,
    ConnectionRejected = 130,
    ServerJoin = 137,
    ServerQuit = 138,
    InitGame = 139,
    SetGravity = 146,
    SetVehicleHealth = 147,
    AttachTrailerToVehicle = 148,
    DetachTrailerFromVehicle = 149,
    QueueDownload = 151,
    SetWeather = 152,
    ExitVehicle = 154,
    UpdateScoresPingsIPs = 155,
    SetPlayerInterior = 156,
    SetPlayerCameraPosition = 157,
    SetPlayerCameraLookAt = 158,
    SetVehiclePosition = 159,
    SetVehicleZAngle = 160,
    SetVehicleParamsForPlayer = 161,
    SetCameraBehindPlayer = 162,
    WorldPlayerRemove = 163,
    WorldVehicleAdd = 164,
    WorldVehicleRemove = 165,
    WorldPlayerDeath = 166,
    DisableVehicleCollisions = 167,
    SetObjectCollision = 169,
    ToggleCameraTarget = 170,
    ShowActor = 171,
    HideActor = 172,
    ApplyActorAnimation = 173,
    ClearActorAnimation = 174,
    SetActorFacingAngle = 175,
    SetActorPosition = 176,
    SetActorHealth = 178,

    PlayAnimation = 86,
    SetPlayerAnimationByIndex = 92,
    SetPlayerAnimationByIndexAlt = 150,
    SetVehicleControllable = 80,
    SpawnPlayerFull = 112,
    SetPlayerSkin = 153,

    DialogResponse = 62,
    GiveTakeDamage = 115,
    SetSpectatorMode = 118,
    CameraTarget = 168,
    GiveActorDamage = 177,
};

constexpr std::uint8_t ToByte(RpcId id) {
    return static_cast<std::uint8_t>(id);
}

constexpr bool HasEmptyBody(RpcId id) {
    switch (id) {
        case RpcId::EnterEditObject:
        case RpcId::CancelEdit:
        case RpcId::DisableCheckpoint:
        case RpcId::DisableRaceCheckpoint:
        case RpcId::GameModeRestart:
        case RpcId::StopAudioStream:
        case RpcId::ForceClassSelection:
        case RpcId::SetCameraBehindPlayer:
        case RpcId::RemovePlayerFromVehicle:
            return true;
        default:
            return false;
    }
}

}
