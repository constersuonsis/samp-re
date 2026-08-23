#pragma once

#include "samp/core/vector3.h"
#include "samp/network/bit_stream.h"
#include "samp/protocol/pool_limits.h"
#include "samp/protocol/protocol_string.h"

#include <array>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace samp::protocol {

struct ChatMessage {
    std::uint16_t playerId = 0;
    std::string text;
};

bool ReadChatMessage(net::BitStream& stream, ChatMessage& message);
void WriteChatMessage(net::BitStream& stream, const ChatMessage& message);

struct ClientMessage {
    std::uint32_t colour = 0;
    std::string text;
};

bool ReadClientMessage(net::BitStream& stream, ClientMessage& message);
void WriteClientMessage(net::BitStream& stream, const ClientMessage& message);

struct DialogHeader {
    std::uint16_t dialogId = 0;
    std::uint8_t style = 0;
    std::string title;
    std::string firstButton;
    std::string secondButton;
};

bool ReadDialogHeader(net::BitStream& stream, DialogHeader& header);
void WriteDialogHeader(net::BitStream& stream, const DialogHeader& header);

enum class DialogButton : std::uint8_t {
    Left = 0,
    Right = 1,
};

inline constexpr std::size_t kMaxDialogInputLength = 128;

struct DialogResponse {
    std::uint16_t dialogId = 0;
    DialogButton button = DialogButton::Left;
    std::int16_t listIndex = -1;

    bool hasInput = false;
    std::string inputText;
};

bool ReadDialogResponse(net::BitStream& stream, DialogResponse& response);
void WriteDialogResponse(net::BitStream& stream, const DialogResponse& response);

inline constexpr std::size_t kMaxDialogBodyLength = 4096;

bool ReadCompressedText(net::BitStream& stream, std::string& text,
                        std::size_t maxLength = kMaxDialogBodyLength);
void WriteCompressedText(net::BitStream& stream, std::string_view text);

struct EnterVehicle {
    std::uint16_t playerId = 0;
    std::uint16_t vehicleId = 0;
    bool asPassenger = false;
};

bool ReadEnterVehicle(net::BitStream& stream, EnterVehicle& message);
void WriteEnterVehicle(net::BitStream& stream, const EnterVehicle& message);

struct ExitVehicle {
    std::uint16_t playerId = 0;
    std::uint16_t vehicleId = 0;
};

bool ReadExitVehicle(net::BitStream& stream, ExitVehicle& message);
void WriteExitVehicle(net::BitStream& stream, const ExitVehicle& message);

struct VehicleDamageStatus {
    std::uint16_t vehicleId = 0;
    std::uint32_t panelStatus = 0;
    std::uint32_t doorStatus = 0;
    std::uint8_t lightStatus = 0;

    std::uint8_t tyreStatus = 0;
};

bool ReadVehicleDamageStatus(net::BitStream& stream, VehicleDamageStatus& message);
void WriteVehicleDamageStatus(net::BitStream& stream, const VehicleDamageStatus& message);

struct CreatePickup {
    std::uint32_t slot = 0;
    std::int32_t model = 0;
    std::int32_t type = 0;
    Vector3 position;
};

bool ReadCreatePickup(net::BitStream& stream, CreatePickup& message);
void WriteCreatePickup(net::BitStream& stream, const CreatePickup& message);

struct DestroyPickup {
    std::uint32_t slot = 0;
};

bool ReadDestroyPickup(net::BitStream& stream, DestroyPickup& message);
void WriteDestroyPickup(net::BitStream& stream, const DestroyPickup& message);

struct SetWeather {
    std::uint8_t weatherId = 0;
};

bool ReadSetWeather(net::BitStream& stream, SetWeather& message);
void WriteSetWeather(net::BitStream& stream, const SetWeather& message);

struct SetWorldTime {
    std::uint8_t hour = 0;
};

bool ReadSetWorldTime(net::BitStream& stream, SetWorldTime& message);
void WriteSetWorldTime(net::BitStream& stream, const SetWorldTime& message);

struct SetPlayerPosition {
    Vector3 position;
};

bool ReadSetPlayerPosition(net::BitStream& stream, SetPlayerPosition& message);
void WriteSetPlayerPosition(net::BitStream& stream, const SetPlayerPosition& message);

struct SetPlayerHealth {
    float health = 0.0f;
};

bool ReadSetPlayerHealth(net::BitStream& stream, SetPlayerHealth& message);
void WriteSetPlayerHealth(net::BitStream& stream, const SetPlayerHealth& message);

struct SetPlayerArmour {
    float armour = 0.0f;
};

bool ReadSetPlayerArmour(net::BitStream& stream, SetPlayerArmour& message);
void WriteSetPlayerArmour(net::BitStream& stream, const SetPlayerArmour& message);

struct SetPlayerColour {
    std::uint16_t playerId = 0;
    std::uint32_t colour = 0;
};

bool ReadSetPlayerColour(net::BitStream& stream, SetPlayerColour& message);
void WriteSetPlayerColour(net::BitStream& stream, const SetPlayerColour& message);

struct SetGravity {
    float gravity = 0.0f;
};

bool ReadSetGravity(net::BitStream& stream, SetGravity& message);
void WriteSetGravity(net::BitStream& stream, const SetGravity& message);

struct SetVehicleHealth {
    std::uint16_t vehicleId = 0;
    float health = 0.0f;
};

bool ReadSetVehicleHealth(net::BitStream& stream, SetVehicleHealth& message);
void WriteSetVehicleHealth(net::BitStream& stream, const SetVehicleHealth& message);

struct SetPlayerInterior {
    std::uint8_t interior = 0;
};

bool ReadSetPlayerInterior(net::BitStream& stream, SetPlayerInterior& message);
void WriteSetPlayerInterior(net::BitStream& stream, const SetPlayerInterior& message);

struct SetPlayerCameraPosition {
    Vector3 position;
};

bool ReadSetPlayerCameraPosition(net::BitStream& stream, SetPlayerCameraPosition& message);
void WriteSetPlayerCameraPosition(net::BitStream& stream, const SetPlayerCameraPosition& message);

enum class CameraCutType : std::uint8_t {
    Move = 1,
    Cut = 2,
};

struct SetPlayerCameraLookAt {
    Vector3 target;
    CameraCutType cutType = CameraCutType::Cut;
};

bool ReadSetPlayerCameraLookAt(net::BitStream& stream, SetPlayerCameraLookAt& message);
void WriteSetPlayerCameraLookAt(net::BitStream& stream, const SetPlayerCameraLookAt& message);

struct SetVehiclePosition {
    std::uint16_t vehicleId = 0;
    Vector3 position;
};

bool ReadSetVehiclePosition(net::BitStream& stream, SetVehiclePosition& message);
void WriteSetVehiclePosition(net::BitStream& stream, const SetVehiclePosition& message);

struct SetVehicleZAngle {
    std::uint16_t vehicleId = 0;
    float angle = 0.0f;
};

bool ReadSetVehicleZAngle(net::BitStream& stream, SetVehicleZAngle& message);
void WriteSetVehicleZAngle(net::BitStream& stream, const SetVehicleZAngle& message);

struct SetRaceCheckpoint {
    std::uint8_t type = 0;
    Vector3 position;
    Vector3 nextPosition;
    float size = 0.0f;
};

bool ReadSetRaceCheckpoint(net::BitStream& stream, SetRaceCheckpoint& message);
void WriteSetRaceCheckpoint(net::BitStream& stream, const SetRaceCheckpoint& message);

inline constexpr std::size_t kMaxChatBubbleLength = 144;

struct ChatBubble {
    std::uint16_t playerId = 0;
    std::uint32_t colour = 0;
    float drawDistance = 0.0f;
    std::int32_t expireTimeMs = 0;
    std::string text;
};

bool ReadChatBubble(net::BitStream& stream, ChatBubble& message);
void WriteChatBubble(net::BitStream& stream, const ChatBubble& message);

inline constexpr std::size_t kWeaponSkillCount = 11;

inline constexpr std::uint8_t kNoTeam = 0xFF;

struct WorldPlayerAdd {
    std::uint16_t playerId = 0;
    std::uint8_t team = kNoTeam;
    std::int32_t skin = 0;
    Vector3 position;
    float rotation = 0.0f;
    std::int32_t colour = 0;
    std::uint8_t fightingStyle = 0;
    std::array<std::uint16_t, kWeaponSkillCount> skillLevels{};

    bool HasTeam() const { return team != kNoTeam; }
};

bool ReadWorldPlayerAdd(net::BitStream& stream, WorldPlayerAdd& message);
void WriteWorldPlayerAdd(net::BitStream& stream, const WorldPlayerAdd& message);

struct ShowActor {
    std::uint16_t actorId = 0;
    std::int32_t modelId = 0;
    Vector3 position;
    float rotation = 0.0f;
    float health = 0.0f;
    std::uint8_t visible = 0;
};

bool ReadShowActor(net::BitStream& stream, ShowActor& message);
void WriteShowActor(net::BitStream& stream, const ShowActor& message);

struct HideActor {
    std::uint16_t actorId = 0;
};

bool ReadHideActor(net::BitStream& stream, HideActor& message);
void WriteHideActor(net::BitStream& stream, const HideActor& message);

inline constexpr std::size_t kShopNameSize = 32;

struct SetPlayerShopName {
    std::string name;

    bool ClosesShop() const { return name.empty(); }
};

bool ReadSetPlayerShopName(net::BitStream& stream, SetPlayerShopName& message);
void WriteSetPlayerShopName(net::BitStream& stream, const SetPlayerShopName& message);

struct SetPlayerDrunkLevel {
    std::int32_t level = 0;
};

bool ReadSetPlayerDrunkLevel(net::BitStream& stream, SetPlayerDrunkLevel& message);
void WriteSetPlayerDrunkLevel(net::BitStream& stream, const SetPlayerDrunkLevel& message);

struct PlayAudioStream {
    std::string url;
    Vector3 position;
    float distance = 0.0f;
    std::uint8_t usePosition = 0;
};

bool ReadPlayAudioStream(net::BitStream& stream, PlayAudioStream& message);
void WritePlayAudioStream(net::BitStream& stream, const PlayAudioStream& message);

struct EditObject {
    bool isPlayerObject = false;
    std::uint16_t objectId = 0;
};

bool ReadEditObject(net::BitStream& stream, EditObject& message);
void WriteEditObject(net::BitStream& stream, const EditObject& message);

struct EditAttachedObject {
    std::int32_t slot = 0;
};

bool ReadEditAttachedObject(net::BitStream& stream, EditAttachedObject& message);
void WriteEditAttachedObject(net::BitStream& stream, const EditAttachedObject& message);

struct AttachObjectToPlayer {
    std::uint16_t objectId = 0;
    std::uint16_t playerId = 0;
    Vector3 offset;
    Vector3 rotation;
};

bool ReadAttachObjectToPlayer(net::BitStream& stream, AttachObjectToPlayer& message);
void WriteAttachObjectToPlayer(net::BitStream& stream, const AttachObjectToPlayer& message);

enum class ClientCheckType : std::uint8_t {

    CurrentModel = 2,

    GameMemory = 5,

    LibraryMemory = 69,

    ModelInfo = 70,

    LoadedModel = 71,

    GameTime = 72,
};

inline constexpr std::uint16_t kMaxClientCheckOffset = 256;
inline constexpr std::uint16_t kMinClientCheckLength = 2;
inline constexpr std::uint16_t kMaxClientCheckLength = 256;

inline constexpr std::uint32_t kGameMemoryStart = 0x400000;
inline constexpr std::uint32_t kGameMemoryEnd = 0x856E00;

inline constexpr std::uint32_t kMaxLibraryOffset = 0xC3500;

struct ClientCheckRequest {
    std::uint8_t type = 0;

    std::uint32_t target = 0;
    std::uint16_t offset = 0;
    std::uint16_t length = 0;

    bool IsWithinBounds() const {
        return offset <= kMaxClientCheckOffset && length >= kMinClientCheckLength &&
               length <= kMaxClientCheckLength;
    }
};

bool ReadClientCheckRequest(net::BitStream& stream, ClientCheckRequest& message);
void WriteClientCheckRequest(net::BitStream& stream, const ClientCheckRequest& message);

struct ClientCheckResponse {
    std::uint8_t type = 0;
    std::uint32_t value = 0;
    std::uint8_t result = 0;
};

bool ReadClientCheckResponse(net::BitStream& stream, ClientCheckResponse& message);
void WriteClientCheckResponse(net::BitStream& stream, const ClientCheckResponse& message);

struct DownloadProgress {
    std::int32_t value = 0;
};

bool ReadDownloadProgress(net::BitStream& stream, DownloadProgress& message);
void WriteDownloadProgress(net::BitStream& stream, const DownloadProgress& message);

struct CancelEdit {};

bool ReadCancelEdit(net::BitStream& stream, CancelEdit& message);
void WriteCancelEdit(net::BitStream& stream, const CancelEdit& message);

inline constexpr std::uint32_t kMaxAttachedObjectSlot = 9;

inline constexpr std::int32_t kMinBoneId = 1;
inline constexpr std::int32_t kMaxBoneId = 18;

struct AttachedObject {
    std::int32_t modelId = 0;
    std::int32_t boneId = 0;
    Vector3 offset;
    Vector3 rotation;
    Vector3 scale;
    std::uint32_t materialColour1 = 0;
    std::uint32_t materialColour2 = 0;

    bool HasValidBone() const { return boneId >= kMinBoneId && boneId <= kMaxBoneId; }
};

struct SetPlayerAttachedObject {
    std::uint16_t playerId = 0;
    std::uint32_t slot = 0;
    bool attached = false;
    AttachedObject object;

    bool HasValidSlot() const { return slot <= kMaxAttachedObjectSlot; }
};

bool ReadSetPlayerAttachedObject(net::BitStream& stream, SetPlayerAttachedObject& message);
void WriteSetPlayerAttachedObject(net::BitStream& stream, const SetPlayerAttachedObject& message);

struct ToggleCameraTarget {
    bool enabled = false;
};

bool ReadToggleCameraTarget(net::BitStream& stream, ToggleCameraTarget& message);
void WriteToggleCameraTarget(net::BitStream& stream, const ToggleCameraTarget& message);

struct ScmEvent {
    std::uint16_t id = 0;
    std::int32_t value1 = 0;
    std::int32_t value2 = 0;
    std::int32_t value3 = 0;
    std::int32_t value4 = 0;
};

bool ReadScmEvent(net::BitStream& stream, ScmEvent& message);
void WriteScmEvent(net::BitStream& stream, const ScmEvent& message);

inline constexpr std::size_t kServerNetStatsSize = 296;

struct ServerNetStats {
    std::array<std::uint8_t, kServerNetStatsSize> data{};
};

bool ReadServerNetStats(net::BitStream& stream, ServerNetStats& message);
void WriteServerNetStats(net::BitStream& stream, const ServerNetStats& message);

struct ApplyActorAnimation {
    std::uint16_t actorId = 0;
    std::string animationLibrary;
    std::string animationName;
    float delta = 0.0f;
    bool loop = false;
    bool lockX = false;
    bool lockY = false;
    bool freeze = false;
    std::int32_t timeMs = 0;
};

bool ReadApplyActorAnimation(net::BitStream& stream, ApplyActorAnimation& message);
void WriteApplyActorAnimation(net::BitStream& stream, const ApplyActorAnimation& message);

struct ClearActorAnimation {
    std::uint16_t actorId = 0;
};

bool ReadClearActorAnimation(net::BitStream& stream, ClearActorAnimation& message);
void WriteClearActorAnimation(net::BitStream& stream, const ClearActorAnimation& message);

struct ClearAnimations {
    std::uint16_t playerId = 0;
};

bool ReadClearAnimations(net::BitStream& stream, ClearAnimations& message);
void WriteClearAnimations(net::BitStream& stream, const ClearAnimations& message);

struct SetActorPosition {
    std::uint16_t actorId = 0;
    Vector3 position;
};

bool ReadSetActorPosition(net::BitStream& stream, SetActorPosition& message);
void WriteSetActorPosition(net::BitStream& stream, const SetActorPosition& message);

struct SetActorHealth {
    std::uint16_t actorId = 0;
    float health = 0.0f;
};

bool ReadSetActorHealth(net::BitStream& stream, SetActorHealth& message);
void WriteSetActorHealth(net::BitStream& stream, const SetActorHealth& message);

struct SetActorFacingAngle {
    std::uint16_t actorId = 0;
    float angle = 0.0f;
};

bool ReadSetActorFacingAngle(net::BitStream& stream, SetActorFacingAngle& message);
void WriteSetActorFacingAngle(net::BitStream& stream, const SetActorFacingAngle& message);

inline constexpr std::size_t kInitGameModelBlockSize = 212;

inline constexpr std::size_t kMaxHostnameLength = 255;

struct InitGame {
    bool flag1 = false;
    bool flag2 = false;
    bool flag3 = false;
    bool flag4 = false;
    std::uint32_t value1 = 0;

    bool stuntBonusEnabled = false;
    std::uint32_t value2 = 0;
    bool flag5 = false;

    bool ignoredFlag = false;

    bool flag6 = false;
    std::uint32_t value3 = 0;

    std::uint16_t playerId = 0;

    bool flag7 = false;
    std::uint32_t value4 = 0;
    std::uint8_t byte1 = 0;
    std::uint8_t weather = 0;

    std::uint32_t gravity = 0;

    bool highRateSync = false;
    std::uint32_t value5 = 0;
    bool flag9 = false;

    std::uint32_t baseSyncIntervalMs = 0;
    std::uint32_t value7 = 0;
    std::uint32_t value8 = 0;
    std::uint32_t value9 = 0;

    std::uint32_t deathMode = 0;

    std::string hostname;

    std::array<std::uint8_t, kInitGameModelBlockSize> modelBlock{};

    std::uint32_t value10 = 0;
};

bool ReadInitGame(net::BitStream& stream, InitGame& message);
void WriteInitGame(net::BitStream& stream, const InitGame& message);

struct ServerJoin {
    std::uint16_t playerId = 0;
    std::int32_t colour = 0;
    std::uint8_t isNpc = 0;
    std::string name;

    bool HasColour() const { return colour != 0; }
};

bool ReadServerJoin(net::BitStream& stream, ServerJoin& message);
void WriteServerJoin(net::BitStream& stream, const ServerJoin& message);

struct ServerQuit {
    std::uint16_t playerId = 0;
    std::uint8_t reason = 0;
};

bool ReadServerQuit(net::BitStream& stream, ServerQuit& message);
void WriteServerQuit(net::BitStream& stream, const ServerQuit& message);

enum class RejectReason : std::uint8_t {
    IncorrectVersion = 1,
    UnacceptableNickname = 2,
    BadModVersion = 3,
    NoPlayerSlot = 4,
};

struct ConnectionRejected {
    std::uint8_t reason = 0;
};

bool ReadConnectionRejected(net::BitStream& stream, ConnectionRejected& message);
void WriteConnectionRejected(net::BitStream& stream, const ConnectionRejected& message);

struct ScoreEntry {
    std::uint16_t playerId = 0;
    std::int32_t score = 0;
    std::int32_t ping = 0;
};

inline constexpr std::size_t kScoreEntrySize = 10;

bool ReadScoreUpdate(net::BitStream& stream, std::vector<ScoreEntry>& entries);
void WriteScoreUpdate(net::BitStream& stream, const std::vector<ScoreEntry>& entries);

inline constexpr std::int32_t kMinVehicleModel = 400;
inline constexpr std::int32_t kMaxVehicleModel = 611;

inline constexpr std::size_t kVehicleModSlots = 14;

inline constexpr std::uint8_t kNoVehicleColour = 0xFF;

inline constexpr std::int32_t kNoModColour = -1;

struct WorldVehicleAdd {
    std::uint16_t vehicleId = 0;
    std::int32_t modelId = 0;
    Vector3 position;
    float rotation = 0.0f;

    std::uint8_t colour1 = kNoVehicleColour;
    std::uint8_t colour2 = kNoVehicleColour;
    float health = 0.0f;

    std::uint8_t interiorColour = 0;

    std::uint32_t doorStatus = 0;
    std::uint32_t panelStatus = 0;
    std::uint8_t lightStatus = 0;
    std::uint8_t tyreStatus = 0;
    std::uint8_t addSiren = 0;

    std::array<std::uint8_t, kVehicleModSlots> modSlots{};

    std::uint8_t paintjob = 0;
    std::int32_t modColour1 = kNoModColour;
    std::int32_t modColour2 = kNoModColour;

    bool HasModelInRange() const {
        return modelId >= kMinVehicleModel && modelId <= kMaxVehicleModel;
    }
    bool HasPaintjob() const { return paintjob != 0; }
    bool HasOwnColours() const {
        return colour1 != kNoVehicleColour || colour2 != kNoVehicleColour;
    }
};

bool ReadWorldVehicleAdd(net::BitStream& stream, WorldVehicleAdd& message);
void WriteWorldVehicleAdd(net::BitStream& stream, const WorldVehicleAdd& message);

struct WorldVehicleRemove {
    std::uint16_t vehicleId = 0;
};

bool ReadWorldVehicleRemove(net::BitStream& stream, WorldVehicleRemove& message);
void WriteWorldVehicleRemove(net::BitStream& stream, const WorldVehicleRemove& message);

struct WorldPlayerDeath {
    std::uint16_t playerId = 0;
};

bool ReadWorldPlayerDeath(net::BitStream& stream, WorldPlayerDeath& message);
void WriteWorldPlayerDeath(net::BitStream& stream, const WorldPlayerDeath& message);

struct WorldPlayerRemove {
    std::uint16_t playerId = 0;
};

bool ReadWorldPlayerRemove(net::BitStream& stream, WorldPlayerRemove& message);
void WriteWorldPlayerRemove(net::BitStream& stream, const WorldPlayerRemove& message);

struct Create3DTextLabel {
    std::uint16_t labelId = 0;
    std::uint32_t colour = 0;
    Vector3 position;
    float drawDistance = 0.0f;

    std::uint8_t testLineOfSight = 0;
    std::uint16_t attachedPlayerId = 0xFFFF;
    std::uint16_t attachedVehicleId = 0xFFFF;
    std::string text;
};

bool ReadCreate3DTextLabel(net::BitStream& stream, Create3DTextLabel& message);
void WriteCreate3DTextLabel(net::BitStream& stream, const Create3DTextLabel& message);

struct Remove3DTextLabel {
    std::uint16_t labelId = 0;
};

bool ReadRemove3DTextLabel(net::BitStream& stream, Remove3DTextLabel& message);
void WriteRemove3DTextLabel(net::BitStream& stream, const Remove3DTextLabel& message);

struct SetCheckpoint {
    Vector3 position;
    float size = 0.0f;
};

bool ReadSetCheckpoint(net::BitStream& stream, SetCheckpoint& message);
void WriteSetCheckpoint(net::BitStream& stream, const SetCheckpoint& message);

struct SetPlayerSkillLevel {
    std::uint16_t playerId = 0;
    std::uint32_t skillType = 0;
    std::uint16_t level = 0;
};

bool ReadSetPlayerSkillLevel(net::BitStream& stream, SetPlayerSkillLevel& message);
void WriteSetPlayerSkillLevel(net::BitStream& stream, const SetPlayerSkillLevel& message);

inline constexpr std::uint16_t kInvalidPlayerId = 0xFFFF;

struct RemoveBuildingForPlayer {
    std::int32_t modelId = 0;
    Vector3 position;
    float radius = 0.0f;
};

bool ReadRemoveBuildingForPlayer(net::BitStream& stream, RemoveBuildingForPlayer& message);
void WriteRemoveBuildingForPlayer(net::BitStream& stream, const RemoveBuildingForPlayer& message);

struct DeathMessage {
    std::uint16_t killerId = kInvalidPlayerId;
    std::uint16_t victimId = kInvalidPlayerId;
    std::uint8_t reason = 0;

    bool HasKiller() const { return killerId != kInvalidPlayerId; }
    bool IsValid() const { return victimId != kInvalidPlayerId; }
};

bool ReadDeathMessage(net::BitStream& stream, DeathMessage& message);
void WriteDeathMessage(net::BitStream& stream, const DeathMessage& message);

struct SelectTextDraw {
    bool enabled = false;
    std::uint32_t hoverColour = 0;
};

bool ReadSelectTextDraw(net::BitStream& stream, SelectTextDraw& message);
void WriteSelectTextDraw(net::BitStream& stream, const SelectTextDraw& message);

inline constexpr std::size_t kMaxNumberPlateLength = 32;

struct SetPlayerMapIcon {
    std::uint8_t iconId = 0;
    Vector3 position;
    std::uint8_t markerType = 0;
    std::int32_t colour = 0;
    std::uint8_t style = 0;
};

bool ReadSetPlayerMapIcon(net::BitStream& stream, SetPlayerMapIcon& message);
void WriteSetPlayerMapIcon(net::BitStream& stream, const SetPlayerMapIcon& message);

struct SetVehicleNumberPlate {
    std::uint16_t vehicleId = 0;
    std::string plate;
};

bool ReadSetVehicleNumberPlate(net::BitStream& stream, SetVehicleNumberPlate& message);
void WriteSetVehicleNumberPlate(net::BitStream& stream, const SetVehicleNumberPlate& message);

struct RemoveVehicleComponent {
    std::uint16_t vehicleId = 0;
    std::uint16_t componentId = 0;
};

bool ReadRemoveVehicleComponent(net::BitStream& stream, RemoveVehicleComponent& message);
void WriteRemoveVehicleComponent(net::BitStream& stream, const RemoveVehicleComponent& message);

struct LinkVehicleToInterior {
    std::uint16_t vehicleId = 0;
    std::uint8_t interiorId = 0;
};

bool ReadLinkVehicleToInterior(net::BitStream& stream, LinkVehicleToInterior& message);
void WriteLinkVehicleToInterior(net::BitStream& stream, const LinkVehicleToInterior& message);

struct AttachTrailerToVehicle {
    std::uint16_t trailerId = 0;
    std::uint16_t vehicleId = 0;
};

bool ReadAttachTrailerToVehicle(net::BitStream& stream, AttachTrailerToVehicle& message);
void WriteAttachTrailerToVehicle(net::BitStream& stream, const AttachTrailerToVehicle& message);

struct DetachTrailerFromVehicle {
    std::uint16_t vehicleId = 0;
};

bool ReadDetachTrailerFromVehicle(net::BitStream& stream, DetachTrailerFromVehicle& message);
void WriteDetachTrailerFromVehicle(net::BitStream& stream, const DetachTrailerFromVehicle& message);

struct SetPlayerWorldBounds {
    float maxX = 0.0f;
    float minX = 0.0f;
    float maxY = 0.0f;
    float minY = 0.0f;
};

bool ReadSetPlayerWorldBounds(net::BitStream& stream, SetPlayerWorldBounds& message);
void WriteSetPlayerWorldBounds(net::BitStream& stream, const SetPlayerWorldBounds& message);

inline constexpr std::size_t kSpawnWeaponSlots = 3;

inline constexpr std::int32_t kNoWeapon = -1;

struct SpawnInfo {
    std::uint8_t team = 0;
    std::uint32_t skin = 0;

    std::uint8_t unused = 0;

    Vector3 position;
    float rotation = 0.0f;

    std::array<std::int32_t, kSpawnWeaponSlots> weapons{kNoWeapon, kNoWeapon, kNoWeapon};
    std::array<std::int32_t, kSpawnWeaponSlots> ammo{};
};

inline constexpr std::size_t kSpawnInfoSize = 46;

bool ReadSpawnInfo(net::BitStream& stream, SpawnInfo& info);
void WriteSpawnInfo(net::BitStream& stream, const SpawnInfo& info);

inline constexpr std::size_t kMaxPlayerNameLength = 24;

struct SetPlayerName {
    std::uint16_t playerId = 0;
    std::string name;
    std::uint8_t result = 0;

    bool Accepted() const { return result == 1; }
};

bool ReadSetPlayerName(net::BitStream& stream, SetPlayerName& message);
void WriteSetPlayerName(net::BitStream& stream, const SetPlayerName& message);

struct SetPlayerTeam {
    std::uint16_t playerId = 0;
    std::uint8_t team = 0;
};

bool ReadSetPlayerTeam(net::BitStream& stream, SetPlayerTeam& message);
void WriteSetPlayerTeam(net::BitStream& stream, const SetPlayerTeam& message);

inline constexpr std::int32_t kMaxGameTextLength = 200;

struct DisplayGameText {
    std::int32_t style = 0;
    std::int32_t durationMs = 0;
    std::string text;
};

bool ReadDisplayGameText(net::BitStream& stream, DisplayGameText& message);
void WriteDisplayGameText(net::BitStream& stream, const DisplayGameText& message);

inline constexpr std::size_t kTextDrawStyleSize = 63;

inline constexpr std::uint8_t kTextDrawPreviewStyle = 4;

struct TextDrawStyle {

    std::uint8_t flags = 0;

    float letterWidth = 0.0f;
    float letterHeight = 0.0f;
    std::uint32_t letterColour = 0;

    float lineWidth = 0.0f;
    float lineHeight = 0.0f;
    std::uint32_t boxColour = 0;

    std::uint8_t shadow = 0;
    std::uint8_t outline = 0;
    std::uint32_t backgroundColour = 0;

    std::uint8_t style = 0;
    std::uint8_t selectable = 0;

    float x = 0.0f;
    float y = 0.0f;

    std::uint16_t previewModel = 0;
    Vector3 previewRotation;
    float previewZoom = 0.0f;
    std::uint16_t previewColour1 = 0;
    std::uint16_t previewColour2 = 0;

    bool IsModelPreview() const { return style == kTextDrawPreviewStyle; }
};

bool ReadTextDrawStyle(net::BitStream& stream, TextDrawStyle& style);
void WriteTextDrawStyle(net::BitStream& stream, const TextDrawStyle& style);

inline constexpr std::uint16_t kMaxTextDrawTextLength = 800;

struct ShowTextDraw {
    std::uint16_t textDrawId = 0;
    TextDrawStyle style;
    std::string text;
};

bool ReadShowTextDraw(net::BitStream& stream, ShowTextDraw& message);
void WriteShowTextDraw(net::BitStream& stream, const ShowTextDraw& message);

struct HideTextDraw {
    std::uint16_t textDrawId = 0;
};

bool ReadHideTextDraw(net::BitStream& stream, HideTextDraw& message);
void WriteHideTextDraw(net::BitStream& stream, const HideTextDraw& message);

struct SetTextDrawString {
    std::uint16_t textDrawId = 0;
    std::string text;
};

bool ReadSetTextDrawString(net::BitStream& stream, SetTextDrawString& message);
void WriteSetTextDrawString(net::BitStream& stream, const SetTextDrawString& message);

struct MoveObject {
    std::uint16_t objectId = 0;

    Vector3 currentPosition;

    Vector3 targetPosition;
    float speed = 0.0f;
    Vector3 targetRotation;
};

bool ReadMoveObject(net::BitStream& stream, MoveObject& message);
void WriteMoveObject(net::BitStream& stream, const MoveObject& message);

struct RequestClassResponse {
    std::uint8_t accepted = 0;
    SpawnInfo spawnInfo;
};

bool ReadRequestClassResponse(net::BitStream& stream, RequestClassResponse& message);
void WriteRequestClassResponse(net::BitStream& stream, const RequestClassResponse& message);

enum class SpawnDecision : std::uint8_t {

    Refused = 0,

    IfPending = 1,

    Immediate = 2,
};

struct RequestSpawnResponse {
    std::uint8_t decision = 0;
};

bool ReadRequestSpawnResponse(net::BitStream& stream, RequestSpawnResponse& message);
void WriteRequestSpawnResponse(net::BitStream& stream, const RequestSpawnResponse& message);

struct SetPlayerSpecialAction {
    std::uint16_t playerId = 0;
    std::uint8_t action = 0;
};

bool ReadSetPlayerSpecialAction(net::BitStream& stream, SetPlayerSpecialAction& message);
void WriteSetPlayerSpecialAction(net::BitStream& stream, const SetPlayerSpecialAction& message);

struct SetVehicleDoors {
    std::uint16_t vehicleId = 0;
    std::uint8_t doorStates = 0;
};

bool ReadSetVehicleDoors(net::BitStream& stream, SetVehicleDoors& message);
void WriteSetVehicleDoors(net::BitStream& stream, const SetVehicleDoors& message);

struct EnableStuntBonus {
    bool enabled = false;
};

bool ReadEnableStuntBonus(net::BitStream& stream, EnableStuntBonus& message);
void WriteEnableStuntBonus(net::BitStream& stream, const EnableStuntBonus& message);

struct PutPlayerInVehicle {
    std::uint16_t vehicleId = 0;
    std::uint8_t seatId = 0;
};

bool ReadPutPlayerInVehicle(net::BitStream& stream, PutPlayerInVehicle& message);
void WritePutPlayerInVehicle(net::BitStream& stream, const PutPlayerInVehicle& message);

struct SetPlayerSkin {
    std::int32_t playerId = 0;
    std::int32_t skinId = 0;
};

bool ReadSetPlayerSkin(net::BitStream& stream, SetPlayerSkin& message);
void WriteSetPlayerSkin(net::BitStream& stream, const SetPlayerSkin& message);

struct SetPlayerPositionFindZ {
    Vector3 position;
};

bool ReadSetPlayerPositionFindZ(net::BitStream& stream, SetPlayerPositionFindZ& message);
void WriteSetPlayerPositionFindZ(net::BitStream& stream, const SetPlayerPositionFindZ& message);

struct SetVehicleParamsForPlayer {
    std::uint16_t vehicleId = 0;
    std::uint8_t value1 = 0;
    std::uint8_t value2 = 0;
};

bool ReadSetVehicleParamsForPlayer(net::BitStream& stream, SetVehicleParamsForPlayer& message);
void WriteSetVehicleParamsForPlayer(net::BitStream& stream,
                                    const SetVehicleParamsForPlayer& message);

struct DisableVehicleCollisions {
    bool disabled = false;
};

bool ReadDisableVehicleCollisions(net::BitStream& stream, DisableVehicleCollisions& message);
void WriteDisableVehicleCollisions(net::BitStream& stream,
                                   const DisableVehicleCollisions& message);

struct EmptyPacket {
    std::uint16_t ignoredValue = 0;
};

bool ReadEmptyPacket(net::BitStream& stream, EmptyPacket& message);
void WriteEmptyPacket(net::BitStream& stream, const EmptyPacket& message);

inline constexpr std::size_t kObjectMovementValues = 5;

struct ApplyObjectMovement {
    std::uint16_t objectId = 0;
    std::array<std::int32_t, kObjectMovementValues> values{};
};

bool ReadApplyObjectMovement(net::BitStream& stream, ApplyObjectMovement& message);
void WriteApplyObjectMovement(net::BitStream& stream, const ApplyObjectMovement& message);

struct ApplyObjectTargetRotation {
    std::uint16_t objectId = 0;
};

bool ReadApplyObjectTargetRotation(net::BitStream& stream, ApplyObjectTargetRotation& message);
void WriteApplyObjectTargetRotation(net::BitStream& stream,
                                    const ApplyObjectTargetRotation& message);

struct SetObjectSpeed {
    std::uint16_t objectId = 0;
    Vector3 speed;
};

bool ReadSetObjectSpeed(net::BitStream& stream, SetObjectSpeed& message);
void WriteSetObjectSpeed(net::BitStream& stream, const SetObjectSpeed& message);

struct SetObjectCollision {
    std::uint16_t objectId = 0;
};

bool ReadSetObjectCollision(net::BitStream& stream, SetObjectCollision& message);
void WriteSetObjectCollision(net::BitStream& stream, const SetObjectCollision& message);

struct SetObjectCurrentRotation {
    std::uint16_t objectId = 0;
    std::int32_t value = 0;
};

bool ReadSetObjectCurrentRotation(net::BitStream& stream, SetObjectCurrentRotation& message);
void WriteSetObjectCurrentRotation(net::BitStream& stream, const SetObjectCurrentRotation& message);

struct ClearObjectMovement {
    std::uint16_t objectId = 0;
};

bool ReadClearObjectMovement(net::BitStream& stream, ClearObjectMovement& message);
void WriteClearObjectMovement(net::BitStream& stream, const ClearObjectMovement& message);

struct SetWidescreen {
    std::uint8_t enabled = 0;
};

bool ReadSetWidescreen(net::BitStream& stream, SetWidescreen& message);
void WriteSetWidescreen(net::BitStream& stream, const SetWidescreen& message);

struct StopObject {
    std::uint16_t objectId = 0;
};

bool ReadStopObject(net::BitStream& stream, StopObject& message);
void WriteStopObject(net::BitStream& stream, const StopObject& message);

inline constexpr std::uint16_t kNoAttachment = 0xFFFF;

struct ObjectAttachment {
    Vector3 offset;
    Vector3 rotation;

    bool syncRotation = false;
};

struct CreateObject {
    std::uint16_t objectId = 0;
    std::int32_t modelId = 0;
    Vector3 position;
    Vector3 rotation;
    float drawDistance = 0.0f;
    bool noCameraCollision = false;

    std::uint16_t attachedVehicleId = kNoAttachment;
    std::uint16_t attachedObjectId = kNoAttachment;
    ObjectAttachment attachment;

    std::uint8_t materialCount = 0;

    bool IsAttached() const {
        return attachedVehicleId != kNoAttachment || attachedObjectId != kNoAttachment;
    }
};

bool ReadCreateObject(net::BitStream& stream, CreateObject& message);
void WriteCreateObject(net::BitStream& stream, const CreateObject& message);

inline constexpr std::size_t kMaxMaterialNameLength = 31;

inline constexpr std::size_t kMaxMaterialTextLength = 2048;

inline constexpr std::uint16_t kMaxMaterialModelId = 20000;

enum class ObjectMaterialType : std::uint8_t {
    Texture = 1,
    Text = 2,
};

struct ObjectMaterialTexture {
    std::uint8_t materialIndex = 0;
    std::uint16_t modelId = 0;
    std::string txdName;
    std::string textureName;
    std::uint32_t colour = 0;

    bool IsValid() const {
        return txdName.size() <= kMaxMaterialNameLength &&
               textureName.size() <= kMaxMaterialNameLength;
    }
};

struct ObjectMaterialText {
    std::uint8_t materialIndex = 0;
    std::uint8_t materialSize = 0;
    std::string fontName;
    std::uint8_t fontSize = 0;
    bool bold = false;
    std::uint32_t fontColour = 0;
    std::uint32_t backgroundColour = 0;
    std::uint8_t alignment = 0;
    std::string text;

    bool IsValid() const {
        return fontName.size() <= kMaxMaterialNameLength && !text.empty();
    }
};

struct ObjectMaterial {
    ObjectMaterialType type = ObjectMaterialType::Texture;
    std::variant<ObjectMaterialTexture, ObjectMaterialText> content;

    bool IsValid() const;
};

bool ReadObjectMaterial(net::BitStream& stream, ObjectMaterial& material);
void WriteObjectMaterial(net::BitStream& stream, const ObjectMaterial& material);

bool ReadObjectMaterials(net::BitStream& stream, std::uint8_t count,
                         std::vector<ObjectMaterial>& materials);

inline constexpr std::uint16_t kNoMaterialModel = 0xFFFF;

enum class MaterialModelRule {

    RejectAboveRange,

    RejectSentinelOnly,
};

constexpr std::int32_t NormaliseMaterialModelId(std::uint16_t modelId, MaterialModelRule rule) {
    if (rule == MaterialModelRule::RejectAboveRange) {
        return modelId > kMaxMaterialModelId ? -1 : static_cast<std::int32_t>(modelId);
    }
    return modelId == kNoMaterialModel ? -1 : static_cast<std::int32_t>(modelId);
}

struct SetObjectMaterial {
    std::uint16_t objectId = 0;
    ObjectMaterial material;
};

bool ReadSetObjectMaterial(net::BitStream& stream, SetObjectMaterial& message);
void WriteSetObjectMaterial(net::BitStream& stream, const SetObjectMaterial& message);

struct DestroyObject {
    std::uint16_t objectId = 0;
};

bool ReadDestroyObject(net::BitStream& stream, DestroyObject& message);
void WriteDestroyObject(net::BitStream& stream, const DestroyObject& message);

struct TogglePlayerControllable {
    bool controllable = false;
};

bool ReadTogglePlayerControllable(net::BitStream& stream, TogglePlayerControllable& message);
void WriteTogglePlayerControllable(net::BitStream& stream, const TogglePlayerControllable& message);

struct PlaySound {
    std::int32_t soundId = 0;
    Vector3 position;
};

bool ReadPlaySound(net::BitStream& stream, PlaySound& message);
void WritePlaySound(net::BitStream& stream, const PlaySound& message);

struct SetPlayerArmedWeapon {
    std::uint32_t weaponId = 0;
};

bool ReadSetPlayerArmedWeapon(net::BitStream& stream, SetPlayerArmedWeapon& message);
void WriteSetPlayerArmedWeapon(net::BitStream& stream, const SetPlayerArmedWeapon& message);

struct SetPlayerWantedLevel {
    std::uint8_t level = 0;
};

bool ReadSetPlayerWantedLevel(net::BitStream& stream, SetPlayerWantedLevel& message);
void WriteSetPlayerWantedLevel(net::BitStream& stream, const SetPlayerWantedLevel& message);

struct GivePlayerWeapon {
    std::int32_t weaponId = 0;
    std::int32_t ammo = 0;
};

bool ReadGivePlayerWeapon(net::BitStream& stream, GivePlayerWeapon& message);
void WriteGivePlayerWeapon(net::BitStream& stream, const GivePlayerWeapon& message);

struct SetPlayerAmmo {
    std::uint8_t weaponSlot = 0;
    std::uint16_t ammo = 0;
};

bool ReadSetPlayerAmmo(net::BitStream& stream, SetPlayerAmmo& message);
void WriteSetPlayerAmmo(net::BitStream& stream, const SetPlayerAmmo& message);

struct RemovePlayerMapIcon {
    std::uint8_t iconId = 0;
};

bool ReadRemovePlayerMapIcon(net::BitStream& stream, RemovePlayerMapIcon& message);
void WriteRemovePlayerMapIcon(net::BitStream& stream, const RemovePlayerMapIcon& message);

struct TogglePlayerSpectating {
    std::uint32_t state = 0;
};

bool ReadTogglePlayerSpectating(net::BitStream& stream, TogglePlayerSpectating& message);
void WriteTogglePlayerSpectating(net::BitStream& stream, const TogglePlayerSpectating& message);

struct SpectateTarget {
    std::uint16_t targetId = 0;
    std::uint8_t mode = 0;
};

bool ReadSpectateTarget(net::BitStream& stream, SpectateTarget& message);
void WriteSpectateTarget(net::BitStream& stream, const SpectateTarget& message);

std::uint8_t CameraModeForSpectate(std::uint8_t mode, bool spectatingVehicle);

}
