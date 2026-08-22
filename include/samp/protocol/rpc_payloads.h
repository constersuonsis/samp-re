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

/// A chat line from another player, or from the local player echoed back.
struct ChatMessage {
    std::uint16_t playerId = 0;
    std::string text;
};

bool ReadChatMessage(net::BitStream& stream, ChatMessage& message);
void WriteChatMessage(net::BitStream& stream, const ChatMessage& message);

/// A coloured line the server prints directly into the chat window.
struct ClientMessage {
    std::uint32_t colour = 0;
    std::string text;
};

bool ReadClientMessage(net::BitStream& stream, ClientMessage& message);
void WriteClientMessage(net::BitStream& stream, const ClientMessage& message);

/// Header of a dialog request.
///
/// The body text is not part of this struct: it travels compressed and is
/// decoded by the script text decoder, so it is read separately from whatever
/// remains in the stream once the header is consumed.
struct DialogHeader {
    std::uint16_t dialogId = 0;
    std::uint8_t style = 0;
    std::string title;
    std::string firstButton;
    std::string secondButton;
};

bool ReadDialogHeader(net::BitStream& stream, DialogHeader& header);
void WriteDialogHeader(net::BitStream& stream, const DialogHeader& header);

/// Longest dialog body the client will decode.
inline constexpr std::size_t kMaxDialogBodyLength = 4096;

/// Reads a Huffman-compressed string: a compressed bit count followed by that
/// many bits of encoded text. Used for the dialog body, which is far too long
/// for the length-prefixed form.
bool ReadCompressedText(net::BitStream& stream, std::string& text,
                        std::size_t maxLength = kMaxDialogBodyLength);
void WriteCompressedText(net::BitStream& stream, std::string_view text);

/// Another player got into a vehicle.
struct EnterVehicle {
    std::uint16_t playerId = 0;
    std::uint16_t vehicleId = 0;
    bool asPassenger = false;
};

bool ReadEnterVehicle(net::BitStream& stream, EnterVehicle& message);
void WriteEnterVehicle(net::BitStream& stream, const EnterVehicle& message);

/// Another player started getting out of a vehicle.
struct ExitVehicle {
    std::uint16_t playerId = 0;
    std::uint16_t vehicleId = 0;
};

bool ReadExitVehicle(net::BitStream& stream, ExitVehicle& message);
void WriteExitVehicle(net::BitStream& stream, const ExitVehicle& message);

/// Visible damage on a vehicle. Each field is a bitmask the game applies to the
/// corresponding group of parts, so partial states survive the round trip.
struct VehicleDamageStatus {
    std::uint16_t vehicleId = 0;
    std::uint32_t panelStatus = 0;
    std::uint32_t doorStatus = 0;
    std::uint8_t lightStatus = 0;
    /// One bit per wheel.
    std::uint8_t tyreStatus = 0;
};

bool ReadVehicleDamageStatus(net::BitStream& stream, VehicleDamageStatus& message);
void WriteVehicleDamageStatus(net::BitStream& stream, const VehicleDamageStatus& message);

/// A pickup appearing in the world at a fixed slot.
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

/// Weather and time both arrive as a single byte applied to global game state.
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

/// Teleports the local player.
struct SetPlayerPosition {
    Vector3 position;
};

bool ReadSetPlayerPosition(net::BitStream& stream, SetPlayerPosition& message);
void WriteSetPlayerPosition(net::BitStream& stream, const SetPlayerPosition& message);

/// Health and armour arrive as floats even though sync quantises them to a
/// nibble; a server-set value is exact until the next sync overwrites it.
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

/// Recolours a player, including the local one.
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

/// How a camera move is applied: only two modes exist, and anything else is
/// treated as the second one.
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

/// A checkpoint on a route: it knows where the next one is so the client can
/// draw an arrow towards it.
struct SetRaceCheckpoint {
    std::uint8_t type = 0;
    Vector3 position;
    Vector3 nextPosition;
    float size = 0.0f;
};

bool ReadSetRaceCheckpoint(net::BitStream& stream, SetRaceCheckpoint& message);
void WriteSetRaceCheckpoint(net::BitStream& stream, const SetRaceCheckpoint& message);

/// Longest chat bubble text. Shorter than every other text limit in the
/// protocol.
inline constexpr std::size_t kMaxChatBubbleLength = 144;

/// Text floating above a player for a while.
struct ChatBubble {
    std::uint16_t playerId = 0;
    std::uint32_t colour = 0;
    float drawDistance = 0.0f;
    std::int32_t expireTimeMs = 0;
    std::string text;
};

bool ReadChatBubble(net::BitStream& stream, ChatBubble& message);
void WriteChatBubble(net::BitStream& stream, const ChatBubble& message);

/// Number of weapon skills tracked per player.
inline constexpr std::size_t kWeaponSkillCount = 11;

/// Team value meaning the player belongs to none.
inline constexpr std::uint8_t kNoTeam = 0xFF;

/// A player entering the streaming radius, with everything needed to spawn them.
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

/// An actor appearing in the world: a static, non-playing character.
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

/// Bytes a shop name occupies on the wire.
inline constexpr std::size_t kShopNameSize = 32;

/// Puts the player inside a shop interface, or clears it.
///
/// The only fixed-width string in the protocol: it always occupies its full
/// thirty-two bytes and ends at the first terminator rather than carrying a
/// length. An empty name closes the interface.
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

/// Starts a radio stream, optionally anchored to a point in the world.
struct PlayAudioStream {
    std::string url;
    Vector3 position;
    float distance = 0.0f;
    std::uint8_t usePosition = 0;
};

bool ReadPlayAudioStream(net::BitStream& stream, PlayAudioStream& message);
void WritePlayAudioStream(net::BitStream& stream, const PlayAudioStream& message);

/// Puts the player into editing mode for one object.
///
/// The flag distinguishing a per-player object from a world one is a single
/// bit, so the message is seventeen bits rather than three bytes.
struct EditObject {
    bool isPlayerObject = false;
    std::uint16_t objectId = 0;
};

bool ReadEditObject(net::BitStream& stream, EditObject& message);
void WriteEditObject(net::BitStream& stream, const EditObject& message);

/// Puts the player into editing mode for one of their attachment slots.
struct EditAttachedObject {
    std::int32_t slot = 0;
};

bool ReadEditAttachedObject(net::BitStream& stream, EditAttachedObject& message);
void WriteEditAttachedObject(net::BitStream& stream, const EditAttachedObject& message);

/// Hangs a world object off a player without using an attachment slot.
struct AttachObjectToPlayer {
    std::uint16_t objectId = 0;
    std::uint16_t playerId = 0;
    Vector3 offset;
    Vector3 rotation;
};

bool ReadAttachObjectToPlayer(net::BitStream& stream, AttachObjectToPlayer& message);
void WriteAttachObjectToPlayer(net::BitStream& stream, const AttachObjectToPlayer& message);

/// What the server is asking the client to look at.
enum class ClientCheckType : std::uint8_t {
    /// The model the player or their vehicle is currently using.
    CurrentModel = 2,
    /// A span inside the game executable's own image.
    GameMemory = 5,
    /// A span inside the client library, addressed from its base.
    LibraryMemory = 69,
    /// A span inside a model's information block.
    ModelInfo = 70,
    /// A span inside a model's loaded data, loading it first if necessary.
    LoadedModel = 71,
    /// A value derived from the running game clock.
    GameTime = 72,
};

/// Bounds every request is checked against before anything is read.
inline constexpr std::uint16_t kMaxClientCheckOffset = 256;
inline constexpr std::uint16_t kMinClientCheckLength = 2;
inline constexpr std::uint16_t kMaxClientCheckLength = 256;

/// Address range accepted for a GameMemory check.
inline constexpr std::uint32_t kGameMemoryStart = 0x400000;
inline constexpr std::uint32_t kGameMemoryEnd = 0x856E00;

/// Highest offset accepted for a LibraryMemory check.
inline constexpr std::uint32_t kMaxLibraryOffset = 0xC3500;

/// The server asking the client to report on something.
struct ClientCheckRequest {
    std::uint8_t type = 0;
    /// An address, a model id or nothing, depending on the check.
    std::uint32_t target = 0;
    std::uint16_t offset = 0;
    std::uint16_t length = 0;

    /// A request outside these bounds is dropped without a reply.
    bool IsWithinBounds() const {
        return offset <= kMaxClientCheckOffset && length >= kMinClientCheckLength &&
               length <= kMaxClientCheckLength;
    }
};

bool ReadClientCheckRequest(net::BitStream& stream, ClientCheckRequest& message);
void WriteClientCheckRequest(net::BitStream& stream, const ClientCheckRequest& message);

/// The client's answer. It travels under the same id as the request, so which
/// of the two a packet holds depends on its direction, not its contents.
struct ClientCheckResponse {
    std::uint8_t type = 0;
    std::uint32_t value = 0;
    std::uint8_t result = 0;
};

bool ReadClientCheckResponse(net::BitStream& stream, ClientCheckResponse& message);
void WriteClientCheckResponse(net::BitStream& stream, const ClientCheckResponse& message);

/// Progress of the asset download, as a single counter.
struct DownloadProgress {
    std::int32_t value = 0;
};

bool ReadDownloadProgress(net::BitStream& stream, DownloadProgress& message);
void WriteDownloadProgress(net::BitStream& stream, const DownloadProgress& message);

/// Leaves object editing mode. The message carries no fields at all: its
/// arrival is the whole instruction. See HasEmptyBody() for the full set of
/// ids that behave this way.
struct CancelEdit {};

bool ReadCancelEdit(net::BitStream& stream, CancelEdit& message);
void WriteCancelEdit(net::BitStream& stream, const CancelEdit& message);

/// Highest attachment slot a player has.
inline constexpr std::uint32_t kMaxAttachedObjectSlot = 9;

/// Range of skeleton bones an object may hang from.
inline constexpr std::int32_t kMinBoneId = 1;
inline constexpr std::int32_t kMaxBoneId = 18;

/// An object carried on a player's skeleton.
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

/// Fills or clears one of a player's attachment slots.
///
/// The object description is only present when something is being attached, so
/// clearing a slot is a much shorter message.
struct SetPlayerAttachedObject {
    std::uint16_t playerId = 0;
    std::uint32_t slot = 0;
    bool attached = false;
    AttachedObject object;

    bool HasValidSlot() const { return slot <= kMaxAttachedObjectSlot; }
};

bool ReadSetPlayerAttachedObject(net::BitStream& stream, SetPlayerAttachedObject& message);
void WriteSetPlayerAttachedObject(net::BitStream& stream, const SetPlayerAttachedObject& message);

/// Turns the camera-target reports on or off. A single bit and nothing else.
struct ToggleCameraTarget {
    bool enabled = false;
};

bool ReadToggleCameraTarget(net::BitStream& stream, ToggleCameraTarget& message);
void WriteToggleCameraTarget(net::BitStream& stream, const ToggleCameraTarget& message);

/// An event raised by the server's script machine.
///
/// The four values are passed straight through to the client's handler without
/// being interpreted, so they keep positional names.
struct ScmEvent {
    std::uint16_t id = 0;
    std::int32_t value1 = 0;
    std::int32_t value2 = 0;
    std::int32_t value3 = 0;
    std::int32_t value4 = 0;
};

bool ReadScmEvent(net::BitStream& stream, ScmEvent& message);
void WriteScmEvent(net::BitStream& stream, const ScmEvent& message);

/// Size of the server statistics blob.
inline constexpr std::size_t kServerNetStatsSize = 296;

/// Server statistics, stored verbatim and rendered elsewhere.
struct ServerNetStats {
    std::array<std::uint8_t, kServerNetStatsSize> data{};
};

bool ReadServerNetStats(net::BitStream& stream, ServerNetStats& message);
void WriteServerNetStats(net::BitStream& stream, const ServerNetStats& message);

/// Plays an animation on an actor.
///
/// The library and clip names are ordinary length-prefixed strings, but the
/// four switches after them are single bits each, so the message is not byte
/// aligned.
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

/// Stops whatever a player is playing and puts them back on their feet.
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

/// Size of the block of per-model settings the init packet ends with.
inline constexpr std::size_t kInitGameModelBlockSize = 212;

/// Longest server name the init packet carries.
inline constexpr std::size_t kMaxHostnameLength = 255;

/// The first packet of a session: server settings, the local player's id and
/// the server name.
///
/// It is bit-packed rather than byte-aligned — the boolean settings occupy a
/// single bit each — so the whole message is not a whole number of bytes.
///
/// Most of the settings are handed straight to the client's configuration block
/// without being interpreted at the point they are read, so only the fields
/// below whose use could be traced carry real names. The rest keep their
/// position in the message under neutral names: the layout is exact even where
/// the meaning is not yet established.
struct InitGame {
    bool flag1 = false;
    bool flag2 = false;
    bool flag3 = false;
    bool flag4 = false;
    std::uint32_t value1 = 0;

    bool stuntBonusEnabled = false;
    std::uint32_t value2 = 0;
    bool flag5 = false;

    /// Read from the wire and then unconditionally forced on, so whatever the
    /// server sends here has no effect.
    bool ignoredFlag = false;

    bool flag6 = false;
    std::uint32_t value3 = 0;

    /// The id the server assigned to this client.
    std::uint16_t playerId = 0;

    bool flag7 = false;
    std::uint32_t value4 = 0;
    std::uint8_t byte1 = 0;
    std::uint8_t weather = 0;

    /// Carried as a raw word; the client reads it straight into a float slot.
    std::uint32_t gravity = 0;

    /// When set, outgoing sync runs at the high fixed rate instead of the
    /// player-count-dependent one.
    bool highRateSync = false;
    std::uint32_t value5 = 0;
    bool flag9 = false;

    /// Floor for the interval between outgoing sync packets. The number of
    /// players nearby is added to it, so a busy server slows each client down.
    std::uint32_t baseSyncIntervalMs = 0;
    std::uint32_t value7 = 0;
    std::uint32_t value8 = 0;
    std::uint32_t value9 = 0;

    /// Selects which set of death handling the client patches in.
    std::uint32_t deathMode = 0;

    std::string hostname;

    std::array<std::uint8_t, kInitGameModelBlockSize> modelBlock{};

    std::uint32_t value10 = 0;
};

bool ReadInitGame(net::BitStream& stream, InitGame& message);
void WriteInitGame(net::BitStream& stream, const InitGame& message);

/// A player joining the server.
struct ServerJoin {
    std::uint16_t playerId = 0;
    std::int32_t colour = 0;
    std::uint8_t isNpc = 0;
    std::string name;

    /// A colour of zero leaves the player on the default one.
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

/// Why the server refused the connection.
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

/// One player's line in the scoreboard.
struct ScoreEntry {
    std::uint16_t playerId = 0;
    std::int32_t score = 0;
    std::int32_t ping = 0;
};

/// Bytes one scoreboard entry occupies.
inline constexpr std::size_t kScoreEntrySize = 10;

/// Reads the scoreboard update.
///
/// The message carries no count: entries simply run to the end of the packet,
/// so the reader consumes whole entries until fewer than one remains.
bool ReadScoreUpdate(net::BitStream& stream, std::vector<ScoreEntry>& entries);
void WriteScoreUpdate(net::BitStream& stream, const std::vector<ScoreEntry>& entries);

/// Range of model ids the client accepts for a vehicle.
inline constexpr std::int32_t kMinVehicleModel = 400;
inline constexpr std::int32_t kMaxVehicleModel = 611;

/// Number of modification slots a vehicle carries.
inline constexpr std::size_t kVehicleModSlots = 14;

/// Colour value meaning "leave the vehicle's own colour alone".
inline constexpr std::uint8_t kNoVehicleColour = 0xFF;

/// Modification colour meaning "not set".
inline constexpr std::int32_t kNoModColour = -1;

/// A vehicle entering the streaming radius.
///
/// The message is two runs back to back: the vehicle's own state, then its
/// modifications. Both are fixed size, so together they always occupy the same
/// number of bytes.
struct WorldVehicleAdd {
    std::uint16_t vehicleId = 0;
    std::int32_t modelId = 0;
    Vector3 position;
    float rotation = 0.0f;

    std::uint8_t colour1 = kNoVehicleColour;
    std::uint8_t colour2 = kNoVehicleColour;
    float health = 0.0f;
    /// Applied only when non-zero.
    std::uint8_t interiorColour = 0;

    std::uint32_t doorStatus = 0;
    std::uint32_t panelStatus = 0;
    std::uint8_t lightStatus = 0;
    std::uint8_t tyreStatus = 0;
    std::uint8_t addSiren = 0;

    /// Fitted parts; a zero slot is empty and the rest are offsets into the
    /// component range rather than component ids on their own.
    std::array<std::uint8_t, kVehicleModSlots> modSlots{};
    /// Zero means no paint job; otherwise the job is one less than the value.
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

/// A player died. Carries nothing but who it was.
struct WorldPlayerDeath {
    std::uint16_t playerId = 0;
};

bool ReadWorldPlayerDeath(net::BitStream& stream, WorldPlayerDeath& message);
void WriteWorldPlayerDeath(net::BitStream& stream, const WorldPlayerDeath& message);

/// A player left the streaming radius and should be despawned.
struct WorldPlayerRemove {
    std::uint16_t playerId = 0;
};

bool ReadWorldPlayerRemove(net::BitStream& stream, WorldPlayerRemove& message);
void WriteWorldPlayerRemove(net::BitStream& stream, const WorldPlayerRemove& message);

/// A floating text label in the world.
///
/// The text is compressed and follows the fixed header, so the message has no
/// single size.
struct Create3DTextLabel {
    std::uint16_t labelId = 0;
    std::uint32_t colour = 0;
    Vector3 position;
    float drawDistance = 0.0f;
    /// When set the label is hidden by walls between it and the camera.
    std::uint8_t testLineOfSight = 0;
    std::uint16_t attachedPlayerId = 0xFFFF;
    std::uint16_t attachedVehicleId = 0xFFFF;
    std::string text;
};

bool ReadCreate3DTextLabel(net::BitStream& stream, Create3DTextLabel& message);
void WriteCreate3DTextLabel(net::BitStream& stream, const Create3DTextLabel& message);

/// Frees a 3D text label slot.
struct Remove3DTextLabel {
    std::uint16_t labelId = 0;
};

bool ReadRemove3DTextLabel(net::BitStream& stream, Remove3DTextLabel& message);
void WriteRemove3DTextLabel(net::BitStream& stream, const Remove3DTextLabel& message);

/// A marker the player has to reach.
///
/// Only one size value travels; the client applies it to all three axes.
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

/// Player id standing for "nobody".
inline constexpr std::uint16_t kInvalidPlayerId = 0xFFFF;

/// Hides every instance of a model inside a sphere.
struct RemoveBuildingForPlayer {
    std::int32_t modelId = 0;
    Vector3 position;
    float radius = 0.0f;
};

bool ReadRemoveBuildingForPlayer(net::BitStream& stream, RemoveBuildingForPlayer& message);
void WriteRemoveBuildingForPlayer(net::BitStream& stream, const RemoveBuildingForPlayer& message);

/// A kill feed entry.
///
/// The victim is always a real player; the killer may be absent, which is how
/// deaths with no attacker are reported.
struct DeathMessage {
    std::uint16_t killerId = kInvalidPlayerId;
    std::uint16_t victimId = kInvalidPlayerId;
    std::uint8_t reason = 0;

    bool HasKiller() const { return killerId != kInvalidPlayerId; }
    bool IsValid() const { return victimId != kInvalidPlayerId; }
};

bool ReadDeathMessage(net::BitStream& stream, DeathMessage& message);
void WriteDeathMessage(net::BitStream& stream, const DeathMessage& message);

/// Turns textdraw selection on or off.
///
/// Unlike every other payload here this one is not byte-aligned: the flag is a
/// single bit followed immediately by the colour, so the whole message is
/// thirty-three bits.
struct SelectTextDraw {
    bool enabled = false;
    std::uint32_t hoverColour = 0;
};

bool ReadSelectTextDraw(net::BitStream& stream, SelectTextDraw& message);
void WriteSelectTextDraw(net::BitStream& stream, const SelectTextDraw& message);

/// Longest number plate the client accepts.
inline constexpr std::size_t kMaxNumberPlateLength = 32;

/// A marker on the radar.
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

/// Hitches a trailer. The trailer comes first on the wire even though the
/// towing vehicle is the one that ends up holding the link.
struct AttachTrailerToVehicle {
    std::uint16_t trailerId = 0;
    std::uint16_t vehicleId = 0;
};

bool ReadAttachTrailerToVehicle(net::BitStream& stream, AttachTrailerToVehicle& message);
void WriteAttachTrailerToVehicle(net::BitStream& stream, const AttachTrailerToVehicle& message);

/// Unhitching names only the towing vehicle.
struct DetachTrailerFromVehicle {
    std::uint16_t vehicleId = 0;
};

bool ReadDetachTrailerFromVehicle(net::BitStream& stream, DetachTrailerFromVehicle& message);
void WriteDetachTrailerFromVehicle(net::BitStream& stream, const DetachTrailerFromVehicle& message);

/// The rectangle the player is kept inside.
///
/// The four values arrive in an order that pairs each axis's upper bound before
/// its lower one, which is not the order the field names would suggest.
struct SetPlayerWorldBounds {
    float maxX = 0.0f;
    float minX = 0.0f;
    float maxY = 0.0f;
    float minY = 0.0f;
};

bool ReadSetPlayerWorldBounds(net::BitStream& stream, SetPlayerWorldBounds& message);
void WriteSetPlayerWorldBounds(net::BitStream& stream, const SetPlayerWorldBounds& message);

/// Number of weapons a player is handed on spawn.
inline constexpr std::size_t kSpawnWeaponSlots = 3;

/// Marker for an empty weapon slot.
inline constexpr std::int32_t kNoWeapon = -1;

/// Everything the player is respawned with.
///
/// The block is 46 bytes and is stored verbatim until the next spawn, then read
/// back field by field.
struct SpawnInfo {
    std::uint8_t team = 0;
    std::uint32_t skin = 0;
    /// Sits between the skin and the position and is not read on spawn.
    std::uint8_t unused = 0;

    Vector3 position;
    float rotation = 0.0f;

    /// Weapons and their ammunition are two separate runs, not interleaved
    /// pairs. Slot i in one lines up with slot i in the other.
    std::array<std::int32_t, kSpawnWeaponSlots> weapons{kNoWeapon, kNoWeapon, kNoWeapon};
    std::array<std::int32_t, kSpawnWeaponSlots> ammo{};
};

/// Size the block occupies on the wire.
inline constexpr std::size_t kSpawnInfoSize = 46;

bool ReadSpawnInfo(net::BitStream& stream, SpawnInfo& info);
void WriteSpawnInfo(net::BitStream& stream, const SpawnInfo& info);

/// Longest nickname the protocol carries.
inline constexpr std::size_t kMaxPlayerNameLength = 24;

/// Renames a player. The trailing flag decides whether the name is taken: the
/// server sends the attempt and its outcome together, and a rejected rename
/// leaves the old name in place.
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

/// Longest and shortest text a game-text banner may carry. Unlike the other
/// string fields an empty one is refused outright.
inline constexpr std::int32_t kMaxGameTextLength = 200;

struct DisplayGameText {
    std::int32_t style = 0;
    std::int32_t durationMs = 0;
    std::string text;
};

bool ReadDisplayGameText(net::BitStream& stream, DisplayGameText& message);
void WriteDisplayGameText(net::BitStream& stream, const DisplayGameText& message);

/// Size of the style block a textdraw carries.
inline constexpr std::size_t kTextDrawStyleSize = 63;

/// Style value that turns a textdraw into a rotating model preview. Only then
/// does the client allocate a texture slot and read the preview fields below.
inline constexpr std::uint8_t kTextDrawPreviewStyle = 4;

/// Appearance of a textdraw.
struct TextDrawStyle {
    /// Bit field; the client reads bits 0x01, 0x02, 0x04, 0x08 and 0x10 and
    /// ignores the rest.
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

    /// Only meaningful when `style` is the preview style.
    std::uint16_t previewModel = 0;
    Vector3 previewRotation;
    float previewZoom = 0.0f;
    std::uint16_t previewColour1 = 0;
    std::uint16_t previewColour2 = 0;

    bool IsModelPreview() const { return style == kTextDrawPreviewStyle; }
};

bool ReadTextDrawStyle(net::BitStream& stream, TextDrawStyle& style);
void WriteTextDrawStyle(net::BitStream& stream, const TextDrawStyle& style);

/// Longest text a textdraw can hold.
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

/// Replaces the text of a textdraw that already exists.
struct SetTextDrawString {
    std::uint16_t textDrawId = 0;
    std::string text;
};

bool ReadSetTextDrawString(net::BitStream& stream, SetTextDrawString& message);
void WriteSetTextDrawString(net::BitStream& stream, const SetTextDrawString& message);

/// Starts an object sliding towards a target.
struct MoveObject {
    std::uint16_t objectId = 0;

    /// Where the server believes the object is now. The client already knows
    /// this and ignores it, but it occupies twelve bytes on the wire and has to
    /// be read for the rest of the message to line up.
    Vector3 currentPosition;

    Vector3 targetPosition;
    float speed = 0.0f;
    Vector3 targetRotation;
};

bool ReadMoveObject(net::BitStream& stream, MoveObject& message);
void WriteMoveObject(net::BitStream& stream, const MoveObject& message);

/// The server's answer to a class request.
///
/// The spawn description is present either way; it is only applied when the
/// request was accepted.
struct RequestClassResponse {
    std::uint8_t accepted = 0;
    SpawnInfo spawnInfo;
};

bool ReadRequestClassResponse(net::BitStream& stream, RequestClassResponse& message);
void WriteRequestClassResponse(net::BitStream& stream, const RequestClassResponse& message);

/// The server's answer to a spawn request.
enum class SpawnDecision : std::uint8_t {
    /// Clears the pending spawn.
    Refused = 0,
    /// Spawns only if a spawn was already pending.
    IfPending = 1,
    /// Spawns unconditionally.
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

/// Sets which of a vehicle's doors stand open, as a bit per door.
struct SetVehicleDoors {
    std::uint16_t vehicleId = 0;
    std::uint8_t doorStates = 0;
};

bool ReadSetVehicleDoors(net::BitStream& stream, SetVehicleDoors& message);
void WriteSetVehicleDoors(net::BitStream& stream, const SetVehicleDoors& message);

/// Switches stunt bonuses. A single bit.
struct EnableStuntBonus {
    bool enabled = false;
};

bool ReadEnableStuntBonus(net::BitStream& stream, EnableStuntBonus& message);
void WriteEnableStuntBonus(net::BitStream& stream, const EnableStuntBonus& message);

/// Seats the local player in a vehicle.
struct PutPlayerInVehicle {
    std::uint16_t vehicleId = 0;
    std::uint8_t seatId = 0;
};

bool ReadPutPlayerInVehicle(net::BitStream& stream, PutPlayerInVehicle& message);
void WritePutPlayerInVehicle(net::BitStream& stream, const PutPlayerInVehicle& message);

/// Changes a player's model. Both fields are full words even though the id
/// never exceeds the player pool.
struct SetPlayerSkin {
    std::int32_t playerId = 0;
    std::int32_t skinId = 0;
};

bool ReadSetPlayerSkin(net::BitStream& stream, SetPlayerSkin& message);
void WriteSetPlayerSkin(net::BitStream& stream, const SetPlayerSkin& message);

/// Teleports the player, letting the client work out the ground height.
///
/// The z that arrives is a starting hint; the client resolves the terrain and
/// drops the player slightly above it.
struct SetPlayerPositionFindZ {
    Vector3 position;
};

bool ReadSetPlayerPositionFindZ(net::BitStream& stream, SetPlayerPositionFindZ& message);
void WriteSetPlayerPositionFindZ(net::BitStream& stream, const SetPlayerPositionFindZ& message);

/// Per-player vehicle parameters. The two trailing bytes are passed on without
/// being interpreted where they are read, so they keep positional names.
struct SetVehicleParamsForPlayer {
    std::uint16_t vehicleId = 0;
    std::uint8_t value1 = 0;
    std::uint8_t value2 = 0;
};

bool ReadSetVehicleParamsForPlayer(net::BitStream& stream, SetVehicleParamsForPlayer& message);
void WriteSetVehicleParamsForPlayer(net::BitStream& stream,
                                    const SetVehicleParamsForPlayer& message);

/// Switches vehicle collisions. A single bit.
struct DisableVehicleCollisions {
    bool disabled = false;
};

bool ReadDisableVehicleCollisions(net::BitStream& stream, DisableVehicleCollisions& message);
void WriteDisableVehicleCollisions(net::BitStream& stream,
                                   const DisableVehicleCollisions& message);

/// A message whose body the client reads and then discards.
///
/// The two bytes are consumed so the stream stays aligned, but nothing is done
/// with them. Dropping the field would leave whatever follows misaligned.
struct EmptyPacket {
    std::uint16_t ignoredValue = 0;
};

bool ReadEmptyPacket(net::BitStream& stream, EmptyPacket& message);
void WriteEmptyPacket(net::BitStream& stream, const EmptyPacket& message);

/// Number of values an object movement update carries after the id.
inline constexpr std::size_t kObjectMovementValues = 5;

/// Drives an object along a movement the client already knows about.
///
/// The five values are handed to the movement code untouched at the point they
/// are read, so they keep positional names.
struct ApplyObjectMovement {
    std::uint16_t objectId = 0;
    std::array<std::int32_t, kObjectMovementValues> values{};
};

bool ReadApplyObjectMovement(net::BitStream& stream, ApplyObjectMovement& message);
void WriteApplyObjectMovement(net::BitStream& stream, const ApplyObjectMovement& message);

/// Applies the rotation an object was already told to move towards.
struct ApplyObjectTargetRotation {
    std::uint16_t objectId = 0;
};

bool ReadApplyObjectTargetRotation(net::BitStream& stream, ApplyObjectTargetRotation& message);
void WriteApplyObjectTargetRotation(net::BitStream& stream,
                                    const ApplyObjectTargetRotation& message);

/// Sets the velocity an object drifts at.
struct SetObjectSpeed {
    std::uint16_t objectId = 0;
    Vector3 speed;
};

bool ReadSetObjectSpeed(net::BitStream& stream, SetObjectSpeed& message);
void WriteSetObjectSpeed(net::BitStream& stream, const SetObjectSpeed& message);

/// Turns an object's collision on.
///
/// There is no value to carry: the message names an object and the client
/// enables its collision unconditionally, so there is no way to switch it back
/// off through this id.
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

/// Switches the letterboxed camera view.
struct SetWidescreen {
    std::uint8_t enabled = 0;
};

bool ReadSetWidescreen(net::BitStream& stream, SetWidescreen& message);
void WriteSetWidescreen(net::BitStream& stream, const SetWidescreen& message);

/// Cancels a movement started by MoveObject.
struct StopObject {
    std::uint16_t objectId = 0;
};

bool ReadStopObject(net::BitStream& stream, StopObject& message);
void WriteStopObject(net::BitStream& stream, const StopObject& message);

/// Value both attachment fields carry when an object stands on its own.
inline constexpr std::uint16_t kNoAttachment = 0xFFFF;

/// Where an object sits relative to whatever it is attached to.
struct ObjectAttachment {
    Vector3 offset;
    Vector3 rotation;
    /// Only honoured when attaching to another object; attaching to a vehicle
    /// ignores it.
    bool syncRotation = false;
};

/// A world object.
///
/// The payload is variable length: the attachment block is only present when
/// the object is attached to something, and a material list of `materialCount`
/// entries follows everything below.
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

/// Longest name the client accepts for a texture, TXD archive, or font.
inline constexpr std::size_t kMaxMaterialNameLength = 31;
/// Longest text a material can carry.
inline constexpr std::size_t kMaxMaterialTextLength = 2048;
/// Model ids above this are treated as absent.
inline constexpr std::uint16_t kMaxMaterialModelId = 20000;

enum class ObjectMaterialType : std::uint8_t {
    Texture = 1,
    Text = 2,
};

/// Replaces one of an object's materials with a texture from a TXD archive.
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

/// Draws text onto one of an object's materials.
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

/// One entry of an object's material list.
struct ObjectMaterial {
    ObjectMaterialType type = ObjectMaterialType::Texture;
    std::variant<ObjectMaterialTexture, ObjectMaterialText> content;

    bool IsValid() const;
};

/// Reads a single material entry.
///
/// A malformed entry still consumes its bytes and comes back marked invalid
/// rather than failing: the list has to stay aligned for the entries after it.
/// Only a truncated stream is reported as a failure.
bool ReadObjectMaterial(net::BitStream& stream, ObjectMaterial& material);
void WriteObjectMaterial(net::BitStream& stream, const ObjectMaterial& material);

bool ReadObjectMaterials(net::BitStream& stream, std::uint8_t count,
                         std::vector<ObjectMaterial>& materials);

/// Marker a material update uses for "no model".
inline constexpr std::uint16_t kNoMaterialModel = 0xFFFF;

/// How a material entry's model id should be read.
///
/// The two messages that carry the entry disagree: the one that creates an
/// object treats anything past the model range as absent, while the one that
/// updates a material accepts every value except a single sentinel. The rule
/// therefore belongs to the message, not to the entry.
enum class MaterialModelRule {
    /// Anything above the model range counts as absent.
    RejectAboveRange,
    /// Only the sentinel counts as absent.
    RejectSentinelOnly,
};

/// Returns the model id to apply, or -1 when the entry names no model.
constexpr std::int32_t NormaliseMaterialModelId(std::uint16_t modelId, MaterialModelRule rule) {
    if (rule == MaterialModelRule::RejectAboveRange) {
        return modelId > kMaxMaterialModelId ? -1 : static_cast<std::int32_t>(modelId);
    }
    return modelId == kNoMaterialModel ? -1 : static_cast<std::int32_t>(modelId);
}

/// Replaces a single material on an existing object.
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

/// Freezes or releases the local player.
struct TogglePlayerControllable {
    bool controllable = false;
};

bool ReadTogglePlayerControllable(net::BitStream& stream, TogglePlayerControllable& message);
void WriteTogglePlayerControllable(net::BitStream& stream, const TogglePlayerControllable& message);

/// A positional sound effect.
struct PlaySound {
    std::int32_t soundId = 0;
    Vector3 position;
};

bool ReadPlaySound(net::BitStream& stream, PlaySound& message);
void WritePlaySound(net::BitStream& stream, const PlaySound& message);

/// Switches the weapon already in the player's inventory.
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

/// Adds a weapon to the player's inventory with its ammunition.
struct GivePlayerWeapon {
    std::int32_t weaponId = 0;
    std::int32_t ammo = 0;
};

bool ReadGivePlayerWeapon(net::BitStream& stream, GivePlayerWeapon& message);
void WriteGivePlayerWeapon(net::BitStream& stream, const GivePlayerWeapon& message);

/// Sets ammunition for a weapon already held. The slot is a byte and the count
/// only sixteen bits, unlike the thirty-two GivePlayerWeapon spends on each.
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

/// Both spectate messages carry a target and a mode byte.
struct SpectateTarget {
    std::uint16_t targetId = 0;
    std::uint8_t mode = 0;
};

bool ReadSpectateTarget(net::BitStream& stream, SpectateTarget& message);
void WriteSpectateTarget(net::BitStream& stream, const SpectateTarget& message);

/// Translates a spectate mode into the camera mode the game is put into.
///
/// The two spectate messages share every mapping except the fallback, which
/// differs by one between watching a player and watching a vehicle.
std::uint8_t CameraModeForSpectate(std::uint8_t mode, bool spectatingVehicle);

}  // namespace samp::protocol
