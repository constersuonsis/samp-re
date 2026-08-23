#include "samp/protocol/rpc_payloads.h"

#include "samp/compression/huffman_tree.h"
#include "samp/protocol/pool_limits.h"

#include <cstring>

namespace samp::protocol {

bool ReadChatMessage(net::BitStream& stream, ChatMessage& message) {
    message = ChatMessage{};
    if (!stream.Read(message.playerId)) {
        return false;
    }
    return ReadString8(stream, message.text);
}

void WriteChatMessage(net::BitStream& stream, const ChatMessage& message) {
    stream.Write(message.playerId);
    WriteString8(stream, message.text);
}

bool ReadClientMessage(net::BitStream& stream, ClientMessage& message) {
    message = ClientMessage{};
    if (!stream.Read(message.colour)) {
        return false;
    }
    return ReadString32(stream, message.text);
}

void WriteClientMessage(net::BitStream& stream, const ClientMessage& message) {
    stream.Write(message.colour);
    WriteString32(stream, message.text);
}

bool ReadDialogHeader(net::BitStream& stream, DialogHeader& header) {
    header = DialogHeader{};

    if (!stream.Read(header.dialogId) || !stream.Read(header.style)) {
        return false;
    }

    return ReadString8(stream, header.title) && ReadString8(stream, header.firstButton) &&
           ReadString8(stream, header.secondButton);
}

void WriteDialogHeader(net::BitStream& stream, const DialogHeader& header) {
    stream.Write(header.dialogId);
    stream.Write(header.style);
    WriteString8(stream, header.title);
    WriteString8(stream, header.firstButton);
    WriteString8(stream, header.secondButton);
}

bool ReadDialogResponse(net::BitStream& stream, DialogResponse& response) {
    response = DialogResponse{};

    std::uint8_t button = 0;
    std::uint8_t inputLength = 0;
    if (!stream.Read(response.dialogId) || !stream.Read(button) ||
        !stream.Read(response.listIndex) || !stream.Read(inputLength)) {
        return false;
    }
    response.button = static_cast<DialogButton>(button);

    if (inputLength == 0) {
        return true;
    }

    std::vector<char> input(inputLength, '\0');
    if (!stream.ReadBytes(input.data(), input.size())) {
        return false;
    }
    response.hasInput = true;
    response.inputText.assign(input.data(),
                              input.back() == '\0' ? input.size() - 1 : input.size());
    return true;
}

void WriteDialogResponse(net::BitStream& stream, const DialogResponse& response) {
    stream.Write(response.dialogId);
    stream.Write(static_cast<std::uint8_t>(response.button));
    stream.Write(response.listIndex);

    std::string input =
        response.inputText.substr(0, response.hasInput ? kMaxDialogInputLength - 1 : 0);
    const std::uint8_t inputLength =
        response.hasInput ? static_cast<std::uint8_t>(input.size() + 1) : 0;
    stream.Write(inputLength);
    if (inputLength != 0) {
        stream.WriteBytes(input.data(), input.size());
        stream.WriteBytes("\0", 1);
    }
}

bool ReadEnterVehicle(net::BitStream& stream, EnterVehicle& message) {
    message = EnterVehicle{};

    std::uint8_t passengerFlag = 0;
    if (!stream.Read(message.playerId) || !stream.Read(message.vehicleId) ||
        !stream.Read(passengerFlag)) {
        return false;
    }

    message.asPassenger = passengerFlag != 0;
    return true;
}

void WriteEnterVehicle(net::BitStream& stream, const EnterVehicle& message) {
    stream.Write(message.playerId);
    stream.Write(message.vehicleId);
    stream.Write(static_cast<std::uint8_t>(message.asPassenger ? 1 : 0));
}

bool ReadExitVehicle(net::BitStream& stream, ExitVehicle& message) {
    message = ExitVehicle{};
    return stream.Read(message.playerId) && stream.Read(message.vehicleId);
}

void WriteExitVehicle(net::BitStream& stream, const ExitVehicle& message) {
    stream.Write(message.playerId);
    stream.Write(message.vehicleId);
}

bool ReadVehicleDamageStatus(net::BitStream& stream, VehicleDamageStatus& message) {
    message = VehicleDamageStatus{};

    return stream.Read(message.vehicleId) && stream.Read(message.panelStatus) &&
           stream.Read(message.doorStatus) && stream.Read(message.lightStatus) &&
           stream.Read(message.tyreStatus);
}

void WriteVehicleDamageStatus(net::BitStream& stream, const VehicleDamageStatus& message) {
    stream.Write(message.vehicleId);
    stream.Write(message.panelStatus);
    stream.Write(message.doorStatus);
    stream.Write(message.lightStatus);
    stream.Write(message.tyreStatus);
}

bool ReadCreatePickup(net::BitStream& stream, CreatePickup& message) {
    message = CreatePickup{};

    if (!stream.Read(message.slot) || !stream.Read(message.model) || !stream.Read(message.type)) {
        return false;
    }

    if (!IsValidPickupSlot(message.slot)) {
        message = CreatePickup{};
        return false;
    }

    return stream.ReadBytes(&message.position, sizeof(message.position));
}

void WriteCreatePickup(net::BitStream& stream, const CreatePickup& message) {
    stream.Write(message.slot);
    stream.Write(message.model);
    stream.Write(message.type);
    stream.WriteBytes(&message.position, sizeof(message.position));
}

bool ReadDestroyPickup(net::BitStream& stream, DestroyPickup& message) {
    message = DestroyPickup{};
    return stream.Read(message.slot);
}

void WriteDestroyPickup(net::BitStream& stream, const DestroyPickup& message) {
    stream.Write(message.slot);
}

bool ReadSetWeather(net::BitStream& stream, SetWeather& message) {
    message = SetWeather{};
    return stream.Read(message.weatherId);
}

void WriteSetWeather(net::BitStream& stream, const SetWeather& message) {
    stream.Write(message.weatherId);
}

bool ReadSetWorldTime(net::BitStream& stream, SetWorldTime& message) {
    message = SetWorldTime{};
    return stream.Read(message.hour);
}

void WriteSetWorldTime(net::BitStream& stream, const SetWorldTime& message) {
    stream.Write(message.hour);
}

bool ReadSetPlayerPosition(net::BitStream& stream, SetPlayerPosition& message) {
    message = SetPlayerPosition{};
    return stream.ReadBytes(&message.position, sizeof(message.position));
}

void WriteSetPlayerPosition(net::BitStream& stream, const SetPlayerPosition& message) {
    stream.WriteBytes(&message.position, sizeof(message.position));
}

bool ReadSetPlayerHealth(net::BitStream& stream, SetPlayerHealth& message) {
    message = SetPlayerHealth{};
    return stream.Read(message.health);
}

void WriteSetPlayerHealth(net::BitStream& stream, const SetPlayerHealth& message) {
    stream.Write(message.health);
}

bool ReadSetPlayerArmour(net::BitStream& stream, SetPlayerArmour& message) {
    message = SetPlayerArmour{};
    return stream.Read(message.armour);
}

void WriteSetPlayerArmour(net::BitStream& stream, const SetPlayerArmour& message) {
    stream.Write(message.armour);
}

bool ReadSetPlayerColour(net::BitStream& stream, SetPlayerColour& message) {
    message = SetPlayerColour{};
    return stream.Read(message.playerId) && stream.Read(message.colour);
}

void WriteSetPlayerColour(net::BitStream& stream, const SetPlayerColour& message) {
    stream.Write(message.playerId);
    stream.Write(message.colour);
}

bool ReadSetGravity(net::BitStream& stream, SetGravity& message) {
    message = SetGravity{};
    return stream.Read(message.gravity);
}

void WriteSetGravity(net::BitStream& stream, const SetGravity& message) {
    stream.Write(message.gravity);
}

bool ReadSetVehicleHealth(net::BitStream& stream, SetVehicleHealth& message) {
    message = SetVehicleHealth{};
    return stream.Read(message.vehicleId) && stream.Read(message.health);
}

void WriteSetVehicleHealth(net::BitStream& stream, const SetVehicleHealth& message) {
    stream.Write(message.vehicleId);
    stream.Write(message.health);
}

bool ReadSetPlayerInterior(net::BitStream& stream, SetPlayerInterior& message) {
    message = SetPlayerInterior{};
    return stream.Read(message.interior);
}

void WriteSetPlayerInterior(net::BitStream& stream, const SetPlayerInterior& message) {
    stream.Write(message.interior);
}

bool ReadSetPlayerCameraPosition(net::BitStream& stream, SetPlayerCameraPosition& message) {
    message = SetPlayerCameraPosition{};
    return stream.ReadBytes(&message.position, sizeof(message.position));
}

void WriteSetPlayerCameraPosition(net::BitStream& stream, const SetPlayerCameraPosition& message) {
    stream.WriteBytes(&message.position, sizeof(message.position));
}

bool ReadSetPlayerCameraLookAt(net::BitStream& stream, SetPlayerCameraLookAt& message) {
    message = SetPlayerCameraLookAt{};

    if (!stream.ReadBytes(&message.target, sizeof(message.target))) {
        return false;
    }

    std::uint8_t cutType = 0;
    if (!stream.Read(cutType)) {
        return false;
    }

    const bool known = cutType == static_cast<std::uint8_t>(CameraCutType::Move) ||
                       cutType == static_cast<std::uint8_t>(CameraCutType::Cut);
    message.cutType = known ? static_cast<CameraCutType>(cutType) : CameraCutType::Cut;
    return true;
}

void WriteSetPlayerCameraLookAt(net::BitStream& stream, const SetPlayerCameraLookAt& message) {
    stream.WriteBytes(&message.target, sizeof(message.target));
    stream.Write(static_cast<std::uint8_t>(message.cutType));
}

bool ReadSetVehiclePosition(net::BitStream& stream, SetVehiclePosition& message) {
    message = SetVehiclePosition{};
    return stream.Read(message.vehicleId) &&
           stream.ReadBytes(&message.position, sizeof(message.position));
}

void WriteSetVehiclePosition(net::BitStream& stream, const SetVehiclePosition& message) {
    stream.Write(message.vehicleId);
    stream.WriteBytes(&message.position, sizeof(message.position));
}

bool ReadSetVehicleZAngle(net::BitStream& stream, SetVehicleZAngle& message) {
    message = SetVehicleZAngle{};
    return stream.Read(message.vehicleId) && stream.Read(message.angle);
}

void WriteSetVehicleZAngle(net::BitStream& stream, const SetVehicleZAngle& message) {
    stream.Write(message.vehicleId);
    stream.Write(message.angle);
}

bool ReadRequestClassResponse(net::BitStream& stream, RequestClassResponse& message) {
    message = RequestClassResponse{};
    return stream.Read(message.accepted) && ReadSpawnInfo(stream, message.spawnInfo);
}

void WriteRequestClassResponse(net::BitStream& stream, const RequestClassResponse& message) {
    stream.Write(message.accepted);
    WriteSpawnInfo(stream, message.spawnInfo);
}

bool ReadRequestSpawnResponse(net::BitStream& stream, RequestSpawnResponse& message) {
    message = RequestSpawnResponse{};
    return stream.Read(message.decision);
}

void WriteRequestSpawnResponse(net::BitStream& stream, const RequestSpawnResponse& message) {
    stream.Write(message.decision);
}

bool ReadSetPlayerSpecialAction(net::BitStream& stream, SetPlayerSpecialAction& message) {
    message = SetPlayerSpecialAction{};
    return stream.Read(message.playerId) && stream.Read(message.action);
}

void WriteSetPlayerSpecialAction(net::BitStream& stream, const SetPlayerSpecialAction& message) {
    stream.Write(message.playerId);
    stream.Write(message.action);
}

bool ReadSetVehicleDoors(net::BitStream& stream, SetVehicleDoors& message) {
    message = SetVehicleDoors{};
    return stream.Read(message.vehicleId) && stream.Read(message.doorStates);
}

void WriteSetVehicleDoors(net::BitStream& stream, const SetVehicleDoors& message) {
    stream.Write(message.vehicleId);
    stream.Write(message.doorStates);
}

bool ReadEnableStuntBonus(net::BitStream& stream, EnableStuntBonus& message) {
    message = EnableStuntBonus{};
    return stream.ReadBit(message.enabled);
}

void WriteEnableStuntBonus(net::BitStream& stream, const EnableStuntBonus& message) {
    stream.WriteBit(message.enabled);
}

bool ReadPutPlayerInVehicle(net::BitStream& stream, PutPlayerInVehicle& message) {
    message = PutPlayerInVehicle{};
    return stream.Read(message.vehicleId) && stream.Read(message.seatId);
}

void WritePutPlayerInVehicle(net::BitStream& stream, const PutPlayerInVehicle& message) {
    stream.Write(message.vehicleId);
    stream.Write(message.seatId);
}

bool ReadSetPlayerSkin(net::BitStream& stream, SetPlayerSkin& message) {
    message = SetPlayerSkin{};
    return stream.Read(message.playerId) && stream.Read(message.skinId);
}

void WriteSetPlayerSkin(net::BitStream& stream, const SetPlayerSkin& message) {
    stream.Write(message.playerId);
    stream.Write(message.skinId);
}

bool ReadSetPlayerPositionFindZ(net::BitStream& stream, SetPlayerPositionFindZ& message) {
    message = SetPlayerPositionFindZ{};
    return stream.ReadBytes(&message.position, sizeof(message.position));
}

void WriteSetPlayerPositionFindZ(net::BitStream& stream, const SetPlayerPositionFindZ& message) {
    stream.WriteBytes(&message.position, sizeof(message.position));
}

bool ReadSetVehicleParamsForPlayer(net::BitStream& stream, SetVehicleParamsForPlayer& message) {
    message = SetVehicleParamsForPlayer{};
    return stream.Read(message.vehicleId) && stream.Read(message.value1) &&
           stream.Read(message.value2);
}

void WriteSetVehicleParamsForPlayer(net::BitStream& stream,
                                    const SetVehicleParamsForPlayer& message) {
    stream.Write(message.vehicleId);
    stream.Write(message.value1);
    stream.Write(message.value2);
}

bool ReadDisableVehicleCollisions(net::BitStream& stream, DisableVehicleCollisions& message) {
    message = DisableVehicleCollisions{};
    return stream.ReadBit(message.disabled);
}

void WriteDisableVehicleCollisions(net::BitStream& stream,
                                   const DisableVehicleCollisions& message) {
    stream.WriteBit(message.disabled);
}

bool ReadEmptyPacket(net::BitStream& stream, EmptyPacket& message) {
    message = EmptyPacket{};
    return stream.Read(message.ignoredValue);
}

void WriteEmptyPacket(net::BitStream& stream, const EmptyPacket& message) {
    stream.Write(message.ignoredValue);
}

bool ReadApplyObjectMovement(net::BitStream& stream, ApplyObjectMovement& message) {
    message = ApplyObjectMovement{};

    if (!stream.Read(message.objectId)) {
        return false;
    }

    for (std::int32_t& value : message.values) {
        if (!stream.Read(value)) {
            message = ApplyObjectMovement{};
            return false;
        }
    }
    return true;
}

void WriteApplyObjectMovement(net::BitStream& stream, const ApplyObjectMovement& message) {
    stream.Write(message.objectId);
    for (std::int32_t value : message.values) {
        stream.Write(value);
    }
}

bool ReadApplyObjectTargetRotation(net::BitStream& stream, ApplyObjectTargetRotation& message) {
    message = ApplyObjectTargetRotation{};
    return stream.Read(message.objectId);
}

void WriteApplyObjectTargetRotation(net::BitStream& stream,
                                    const ApplyObjectTargetRotation& message) {
    stream.Write(message.objectId);
}

bool ReadSetObjectSpeed(net::BitStream& stream, SetObjectSpeed& message) {
    message = SetObjectSpeed{};
    return stream.Read(message.objectId) &&
           stream.ReadBytes(&message.speed, sizeof(message.speed));
}

void WriteSetObjectSpeed(net::BitStream& stream, const SetObjectSpeed& message) {
    stream.Write(message.objectId);
    stream.WriteBytes(&message.speed, sizeof(message.speed));
}

bool ReadSetObjectCollision(net::BitStream& stream, SetObjectCollision& message) {
    message = SetObjectCollision{};
    return stream.Read(message.objectId);
}

void WriteSetObjectCollision(net::BitStream& stream, const SetObjectCollision& message) {
    stream.Write(message.objectId);
}

bool ReadSetObjectCurrentRotation(net::BitStream& stream, SetObjectCurrentRotation& message) {
    message = SetObjectCurrentRotation{};
    return stream.Read(message.objectId) && stream.Read(message.value);
}

void WriteSetObjectCurrentRotation(net::BitStream& stream,
                                   const SetObjectCurrentRotation& message) {
    stream.Write(message.objectId);
    stream.Write(message.value);
}

bool ReadClearObjectMovement(net::BitStream& stream, ClearObjectMovement& message) {
    message = ClearObjectMovement{};
    return stream.Read(message.objectId);
}

void WriteClearObjectMovement(net::BitStream& stream, const ClearObjectMovement& message) {
    stream.Write(message.objectId);
}

bool ReadSetWidescreen(net::BitStream& stream, SetWidescreen& message) {
    message = SetWidescreen{};
    return stream.Read(message.enabled);
}

void WriteSetWidescreen(net::BitStream& stream, const SetWidescreen& message) {
    stream.Write(message.enabled);
}

bool ReadStopObject(net::BitStream& stream, StopObject& message) {
    message = StopObject{};
    return stream.Read(message.objectId);
}

void WriteStopObject(net::BitStream& stream, const StopObject& message) {
    stream.Write(message.objectId);
}

bool ReadSetRaceCheckpoint(net::BitStream& stream, SetRaceCheckpoint& message) {
    message = SetRaceCheckpoint{};

    return stream.Read(message.type) &&
           stream.ReadBytes(&message.position, sizeof(message.position)) &&
           stream.ReadBytes(&message.nextPosition, sizeof(message.nextPosition)) &&
           stream.Read(message.size);
}

void WriteSetRaceCheckpoint(net::BitStream& stream, const SetRaceCheckpoint& message) {
    stream.Write(message.type);
    stream.WriteBytes(&message.position, sizeof(message.position));
    stream.WriteBytes(&message.nextPosition, sizeof(message.nextPosition));
    stream.Write(message.size);
}

bool ReadChatBubble(net::BitStream& stream, ChatBubble& message) {
    message = ChatBubble{};

    if (!stream.Read(message.playerId) || !stream.Read(message.colour) ||
        !stream.Read(message.drawDistance) || !stream.Read(message.expireTimeMs)) {
        return false;
    }

    std::uint8_t length = 0;
    if (!stream.Read(length) || length > kMaxChatBubbleLength) {
        return false;
    }

    if (length > 0) {
        message.text.resize(length);
        if (!stream.ReadBytes(message.text.data(), length)) {
            message = ChatBubble{};
            return false;
        }
    }
    return true;
}

void WriteChatBubble(net::BitStream& stream, const ChatBubble& message) {
    const auto length =
        static_cast<std::uint8_t>(std::min(message.text.size(), kMaxChatBubbleLength));

    stream.Write(message.playerId);
    stream.Write(message.colour);
    stream.Write(message.drawDistance);
    stream.Write(message.expireTimeMs);
    stream.Write(length);
    stream.WriteBytes(message.text.data(), length);
}

bool ReadWorldPlayerAdd(net::BitStream& stream, WorldPlayerAdd& message) {
    message = WorldPlayerAdd{};

    if (!stream.Read(message.playerId) || !stream.Read(message.team) ||
        !stream.Read(message.skin) ||
        !stream.ReadBytes(&message.position, sizeof(message.position)) ||
        !stream.Read(message.rotation) || !stream.Read(message.colour) ||
        !stream.Read(message.fightingStyle)) {
        return false;
    }

    for (std::uint16_t& skill : message.skillLevels) {
        if (!stream.Read(skill)) {
            message = WorldPlayerAdd{};
            return false;
        }
    }
    return true;
}

void WriteWorldPlayerAdd(net::BitStream& stream, const WorldPlayerAdd& message) {
    stream.Write(message.playerId);
    stream.Write(message.team);
    stream.Write(message.skin);
    stream.WriteBytes(&message.position, sizeof(message.position));
    stream.Write(message.rotation);
    stream.Write(message.colour);
    stream.Write(message.fightingStyle);

    for (std::uint16_t skill : message.skillLevels) {
        stream.Write(skill);
    }
}

bool ReadShowActor(net::BitStream& stream, ShowActor& message) {
    message = ShowActor{};

    return stream.Read(message.actorId) && stream.Read(message.modelId) &&
           stream.ReadBytes(&message.position, sizeof(message.position)) &&
           stream.Read(message.rotation) && stream.Read(message.health) &&
           stream.Read(message.visible);
}

void WriteShowActor(net::BitStream& stream, const ShowActor& message) {
    stream.Write(message.actorId);
    stream.Write(message.modelId);
    stream.WriteBytes(&message.position, sizeof(message.position));
    stream.Write(message.rotation);
    stream.Write(message.health);
    stream.Write(message.visible);
}

bool ReadHideActor(net::BitStream& stream, HideActor& message) {
    message = HideActor{};
    return stream.Read(message.actorId);
}

void WriteHideActor(net::BitStream& stream, const HideActor& message) {
    stream.Write(message.actorId);
}

bool ReadSetPlayerShopName(net::BitStream& stream, SetPlayerShopName& message) {
    message = SetPlayerShopName{};

    char buffer[kShopNameSize] = {};
    if (!stream.ReadBytes(buffer, sizeof(buffer))) {
        return false;
    }

    const auto* end = static_cast<const char*>(std::memchr(buffer, '\0', sizeof(buffer)));
    message.name.assign(buffer, end != nullptr ? static_cast<std::size_t>(end - buffer)
                                               : sizeof(buffer));
    return true;
}

void WriteSetPlayerShopName(net::BitStream& stream, const SetPlayerShopName& message) {
    char buffer[kShopNameSize] = {};
    const std::size_t length = std::min(message.name.size(), kShopNameSize - 1);
    std::memcpy(buffer, message.name.data(), length);

    stream.WriteBytes(buffer, sizeof(buffer));
}

bool ReadSetPlayerDrunkLevel(net::BitStream& stream, SetPlayerDrunkLevel& message) {
    message = SetPlayerDrunkLevel{};
    return stream.Read(message.level);
}

void WriteSetPlayerDrunkLevel(net::BitStream& stream, const SetPlayerDrunkLevel& message) {
    stream.Write(message.level);
}

bool ReadPlayAudioStream(net::BitStream& stream, PlayAudioStream& message) {
    message = PlayAudioStream{};

    return ReadString8(stream, message.url) &&
           stream.ReadBytes(&message.position, sizeof(message.position)) &&
           stream.Read(message.distance) && stream.Read(message.usePosition);
}

void WritePlayAudioStream(net::BitStream& stream, const PlayAudioStream& message) {
    WriteString8(stream, message.url);
    stream.WriteBytes(&message.position, sizeof(message.position));
    stream.Write(message.distance);
    stream.Write(message.usePosition);
}

bool ReadEditObject(net::BitStream& stream, EditObject& message) {
    message = EditObject{};
    return stream.ReadBit(message.isPlayerObject) && stream.Read(message.objectId);
}

void WriteEditObject(net::BitStream& stream, const EditObject& message) {
    stream.WriteBit(message.isPlayerObject);
    stream.Write(message.objectId);
}

bool ReadEditAttachedObject(net::BitStream& stream, EditAttachedObject& message) {
    message = EditAttachedObject{};
    return stream.Read(message.slot);
}

void WriteEditAttachedObject(net::BitStream& stream, const EditAttachedObject& message) {
    stream.Write(message.slot);
}

bool ReadAttachObjectToPlayer(net::BitStream& stream, AttachObjectToPlayer& message) {
    message = AttachObjectToPlayer{};

    return stream.Read(message.objectId) && stream.Read(message.playerId) &&
           stream.ReadBytes(&message.offset, sizeof(message.offset)) &&
           stream.ReadBytes(&message.rotation, sizeof(message.rotation));
}

void WriteAttachObjectToPlayer(net::BitStream& stream, const AttachObjectToPlayer& message) {
    stream.Write(message.objectId);
    stream.Write(message.playerId);
    stream.WriteBytes(&message.offset, sizeof(message.offset));
    stream.WriteBytes(&message.rotation, sizeof(message.rotation));
}

bool ReadClientCheckRequest(net::BitStream& stream, ClientCheckRequest& message) {
    message = ClientCheckRequest{};

    return stream.Read(message.type) && stream.Read(message.target) &&
           stream.Read(message.offset) && stream.Read(message.length);
}

void WriteClientCheckRequest(net::BitStream& stream, const ClientCheckRequest& message) {
    stream.Write(message.type);
    stream.Write(message.target);
    stream.Write(message.offset);
    stream.Write(message.length);
}

bool ReadClientCheckResponse(net::BitStream& stream, ClientCheckResponse& message) {
    message = ClientCheckResponse{};

    return stream.Read(message.type) && stream.Read(message.value) && stream.Read(message.result);
}

void WriteClientCheckResponse(net::BitStream& stream, const ClientCheckResponse& message) {
    stream.Write(message.type);
    stream.Write(message.value);
    stream.Write(message.result);
}

bool ReadDownloadProgress(net::BitStream& stream, DownloadProgress& message) {
    message = DownloadProgress{};
    return stream.Read(message.value);
}

void WriteDownloadProgress(net::BitStream& stream, const DownloadProgress& message) {
    stream.Write(message.value);
}

bool ReadCancelEdit(net::BitStream& stream, CancelEdit& message) {
    (void)stream;
    (void)message;
    return true;
}

void WriteCancelEdit(net::BitStream& stream, const CancelEdit& message) {
    (void)stream;
    (void)message;
}

bool ReadSetPlayerAttachedObject(net::BitStream& stream, SetPlayerAttachedObject& message) {
    message = SetPlayerAttachedObject{};

    if (!stream.Read(message.playerId) || !stream.Read(message.slot) ||
        !stream.ReadBit(message.attached)) {
        return false;
    }

    if (!message.attached) {
        return true;
    }

    AttachedObject& object = message.object;
    return stream.Read(object.modelId) && stream.Read(object.boneId) &&
           stream.ReadBytes(&object.offset, sizeof(object.offset)) &&
           stream.ReadBytes(&object.rotation, sizeof(object.rotation)) &&
           stream.ReadBytes(&object.scale, sizeof(object.scale)) &&
           stream.Read(object.materialColour1) && stream.Read(object.materialColour2);
}

void WriteSetPlayerAttachedObject(net::BitStream& stream, const SetPlayerAttachedObject& message) {
    stream.Write(message.playerId);
    stream.Write(message.slot);
    stream.WriteBit(message.attached);

    if (!message.attached) {
        return;
    }

    const AttachedObject& object = message.object;
    stream.Write(object.modelId);
    stream.Write(object.boneId);
    stream.WriteBytes(&object.offset, sizeof(object.offset));
    stream.WriteBytes(&object.rotation, sizeof(object.rotation));
    stream.WriteBytes(&object.scale, sizeof(object.scale));
    stream.Write(object.materialColour1);
    stream.Write(object.materialColour2);
}

bool ReadToggleCameraTarget(net::BitStream& stream, ToggleCameraTarget& message) {
    message = ToggleCameraTarget{};
    return stream.ReadBit(message.enabled);
}

void WriteToggleCameraTarget(net::BitStream& stream, const ToggleCameraTarget& message) {
    stream.WriteBit(message.enabled);
}

bool ReadScmEvent(net::BitStream& stream, ScmEvent& message) {
    message = ScmEvent{};
    return stream.Read(message.id) && stream.Read(message.value1) && stream.Read(message.value2) &&
           stream.Read(message.value3) && stream.Read(message.value4);
}

void WriteScmEvent(net::BitStream& stream, const ScmEvent& message) {
    stream.Write(message.id);
    stream.Write(message.value1);
    stream.Write(message.value2);
    stream.Write(message.value3);
    stream.Write(message.value4);
}

bool ReadServerNetStats(net::BitStream& stream, ServerNetStats& message) {
    message = ServerNetStats{};
    return stream.ReadBytes(message.data.data(), message.data.size());
}

void WriteServerNetStats(net::BitStream& stream, const ServerNetStats& message) {
    stream.WriteBytes(message.data.data(), message.data.size());
}

bool ReadApplyActorAnimation(net::BitStream& stream, ApplyActorAnimation& message) {
    message = ApplyActorAnimation{};

    if (!stream.Read(message.actorId) || !ReadString8(stream, message.animationLibrary) ||
        !ReadString8(stream, message.animationName) || !stream.Read(message.delta)) {
        return false;
    }

    if (!stream.ReadBit(message.loop) || !stream.ReadBit(message.lockX) ||
        !stream.ReadBit(message.lockY) || !stream.ReadBit(message.freeze)) {
        return false;
    }

    return stream.Read(message.timeMs);
}

void WriteApplyActorAnimation(net::BitStream& stream, const ApplyActorAnimation& message) {
    stream.Write(message.actorId);
    WriteString8(stream, message.animationLibrary);
    WriteString8(stream, message.animationName);
    stream.Write(message.delta);
    stream.WriteBit(message.loop);
    stream.WriteBit(message.lockX);
    stream.WriteBit(message.lockY);
    stream.WriteBit(message.freeze);
    stream.Write(message.timeMs);
}

bool ReadClearActorAnimation(net::BitStream& stream, ClearActorAnimation& message) {
    message = ClearActorAnimation{};
    return stream.Read(message.actorId);
}

void WriteClearActorAnimation(net::BitStream& stream, const ClearActorAnimation& message) {
    stream.Write(message.actorId);
}

bool ReadClearAnimations(net::BitStream& stream, ClearAnimations& message) {
    message = ClearAnimations{};
    return stream.Read(message.playerId);
}

void WriteClearAnimations(net::BitStream& stream, const ClearAnimations& message) {
    stream.Write(message.playerId);
}

bool ReadSetActorPosition(net::BitStream& stream, SetActorPosition& message) {
    message = SetActorPosition{};
    return stream.Read(message.actorId) &&
           stream.ReadBytes(&message.position, sizeof(message.position));
}

void WriteSetActorPosition(net::BitStream& stream, const SetActorPosition& message) {
    stream.Write(message.actorId);
    stream.WriteBytes(&message.position, sizeof(message.position));
}

bool ReadSetActorHealth(net::BitStream& stream, SetActorHealth& message) {
    message = SetActorHealth{};
    return stream.Read(message.actorId) && stream.Read(message.health);
}

void WriteSetActorHealth(net::BitStream& stream, const SetActorHealth& message) {
    stream.Write(message.actorId);
    stream.Write(message.health);
}

bool ReadSetActorFacingAngle(net::BitStream& stream, SetActorFacingAngle& message) {
    message = SetActorFacingAngle{};
    return stream.Read(message.actorId) && stream.Read(message.angle);
}

void WriteSetActorFacingAngle(net::BitStream& stream, const SetActorFacingAngle& message) {
    stream.Write(message.actorId);
    stream.Write(message.angle);
}

bool ReadInitGame(net::BitStream& stream, InitGame& message) {
    message = InitGame{};

    if (!stream.ReadBit(message.flag1) || !stream.ReadBit(message.flag2) ||
        !stream.ReadBit(message.flag3) || !stream.ReadBit(message.flag4) ||
        !stream.Read(message.value1) || !stream.ReadBit(message.stuntBonusEnabled) ||
        !stream.Read(message.value2) || !stream.ReadBit(message.flag5) ||
        !stream.ReadBit(message.ignoredFlag) || !stream.ReadBit(message.flag6) ||
        !stream.Read(message.value3) || !stream.Read(message.playerId) ||
        !stream.ReadBit(message.flag7) || !stream.Read(message.value4) ||
        !stream.Read(message.byte1) || !stream.Read(message.weather) ||
        !stream.Read(message.gravity) || !stream.ReadBit(message.highRateSync) ||
        !stream.Read(message.value5) || !stream.ReadBit(message.flag9) ||
        !stream.Read(message.baseSyncIntervalMs) || !stream.Read(message.value7) ||
        !stream.Read(message.value8) || !stream.Read(message.value9) ||
        !stream.Read(message.deathMode)) {
        return false;
    }

    std::uint8_t hostnameLength = 0;
    if (!stream.Read(hostnameLength)) {
        return false;
    }

    if (hostnameLength > 0) {
        message.hostname.resize(hostnameLength);
        if (!stream.ReadBytes(message.hostname.data(), hostnameLength)) {
            message = InitGame{};
            return false;
        }
    }

    if (!stream.ReadBytes(message.modelBlock.data(), message.modelBlock.size())) {
        message = InitGame{};
        return false;
    }

    return stream.Read(message.value10);
}

void WriteInitGame(net::BitStream& stream, const InitGame& message) {
    stream.WriteBit(message.flag1);
    stream.WriteBit(message.flag2);
    stream.WriteBit(message.flag3);
    stream.WriteBit(message.flag4);
    stream.Write(message.value1);
    stream.WriteBit(message.stuntBonusEnabled);
    stream.Write(message.value2);
    stream.WriteBit(message.flag5);
    stream.WriteBit(message.ignoredFlag);
    stream.WriteBit(message.flag6);
    stream.Write(message.value3);
    stream.Write(message.playerId);
    stream.WriteBit(message.flag7);
    stream.Write(message.value4);
    stream.Write(message.byte1);
    stream.Write(message.weather);
    stream.Write(message.gravity);
    stream.WriteBit(message.highRateSync);
    stream.Write(message.value5);
    stream.WriteBit(message.flag9);
    stream.Write(message.baseSyncIntervalMs);
    stream.Write(message.value7);
    stream.Write(message.value8);
    stream.Write(message.value9);
    stream.Write(message.deathMode);

    const auto hostnameLength =
        static_cast<std::uint8_t>(std::min(message.hostname.size(), kMaxHostnameLength));
    stream.Write(hostnameLength);
    stream.WriteBytes(message.hostname.data(), hostnameLength);

    stream.WriteBytes(message.modelBlock.data(), message.modelBlock.size());
    stream.Write(message.value10);
}

bool ReadServerJoin(net::BitStream& stream, ServerJoin& message) {
    message = ServerJoin{};

    if (!stream.Read(message.playerId) || !stream.Read(message.colour) ||
        !stream.Read(message.isNpc)) {
        return false;
    }

    std::uint8_t length = 0;
    if (!stream.Read(length) || length > kMaxPlayerNameLength) {
        return false;
    }

    if (length > 0) {
        message.name.resize(length);
        if (!stream.ReadBytes(message.name.data(), length)) {
            message = ServerJoin{};
            return false;
        }
    }
    return true;
}

void WriteServerJoin(net::BitStream& stream, const ServerJoin& message) {
    const auto length =
        static_cast<std::uint8_t>(std::min(message.name.size(), kMaxPlayerNameLength));

    stream.Write(message.playerId);
    stream.Write(message.colour);
    stream.Write(message.isNpc);
    stream.Write(length);
    stream.WriteBytes(message.name.data(), length);
}

bool ReadServerQuit(net::BitStream& stream, ServerQuit& message) {
    message = ServerQuit{};
    return stream.Read(message.playerId) && stream.Read(message.reason);
}

void WriteServerQuit(net::BitStream& stream, const ServerQuit& message) {
    stream.Write(message.playerId);
    stream.Write(message.reason);
}

bool ReadConnectionRejected(net::BitStream& stream, ConnectionRejected& message) {
    message = ConnectionRejected{};
    return stream.Read(message.reason);
}

void WriteConnectionRejected(net::BitStream& stream, const ConnectionRejected& message) {
    stream.Write(message.reason);
}

bool ReadScoreUpdate(net::BitStream& stream, std::vector<ScoreEntry>& entries) {
    entries.clear();

    const int entryBits = static_cast<int>(kScoreEntrySize) * 8;
    while (stream.GetNumberOfUnreadBits() >= entryBits) {
        ScoreEntry entry;
        if (!stream.Read(entry.playerId) || !stream.Read(entry.score) ||
            !stream.Read(entry.ping)) {
            entries.clear();
            return false;
        }
        entries.push_back(entry);
    }

    return true;
}

void WriteScoreUpdate(net::BitStream& stream, const std::vector<ScoreEntry>& entries) {
    for (const ScoreEntry& entry : entries) {
        stream.Write(entry.playerId);
        stream.Write(entry.score);
        stream.Write(entry.ping);
    }
}

bool ReadWorldVehicleAdd(net::BitStream& stream, WorldVehicleAdd& message) {
    message = WorldVehicleAdd{};

    if (!stream.Read(message.vehicleId) || !stream.Read(message.modelId) ||
        !stream.ReadBytes(&message.position, sizeof(message.position)) ||
        !stream.Read(message.rotation) || !stream.Read(message.colour1) ||
        !stream.Read(message.colour2) || !stream.Read(message.health) ||
        !stream.Read(message.interiorColour) || !stream.Read(message.doorStatus) ||
        !stream.Read(message.panelStatus) || !stream.Read(message.lightStatus) ||
        !stream.Read(message.tyreStatus) || !stream.Read(message.addSiren)) {
        return false;
    }

    for (std::uint8_t& slot : message.modSlots) {
        if (!stream.Read(slot)) {
            message = WorldVehicleAdd{};
            return false;
        }
    }

    return stream.Read(message.paintjob) && stream.Read(message.modColour1) &&
           stream.Read(message.modColour2);
}

void WriteWorldVehicleAdd(net::BitStream& stream, const WorldVehicleAdd& message) {
    stream.Write(message.vehicleId);
    stream.Write(message.modelId);
    stream.WriteBytes(&message.position, sizeof(message.position));
    stream.Write(message.rotation);
    stream.Write(message.colour1);
    stream.Write(message.colour2);
    stream.Write(message.health);
    stream.Write(message.interiorColour);
    stream.Write(message.doorStatus);
    stream.Write(message.panelStatus);
    stream.Write(message.lightStatus);
    stream.Write(message.tyreStatus);
    stream.Write(message.addSiren);

    for (std::uint8_t slot : message.modSlots) {
        stream.Write(slot);
    }

    stream.Write(message.paintjob);
    stream.Write(message.modColour1);
    stream.Write(message.modColour2);
}

bool ReadWorldVehicleRemove(net::BitStream& stream, WorldVehicleRemove& message) {
    message = WorldVehicleRemove{};
    return stream.Read(message.vehicleId);
}

void WriteWorldVehicleRemove(net::BitStream& stream, const WorldVehicleRemove& message) {
    stream.Write(message.vehicleId);
}

bool ReadWorldPlayerDeath(net::BitStream& stream, WorldPlayerDeath& message) {
    message = WorldPlayerDeath{};
    return stream.Read(message.playerId);
}

void WriteWorldPlayerDeath(net::BitStream& stream, const WorldPlayerDeath& message) {
    stream.Write(message.playerId);
}

bool ReadWorldPlayerRemove(net::BitStream& stream, WorldPlayerRemove& message) {
    message = WorldPlayerRemove{};
    return stream.Read(message.playerId);
}

void WriteWorldPlayerRemove(net::BitStream& stream, const WorldPlayerRemove& message) {
    stream.Write(message.playerId);
}

bool ReadCreate3DTextLabel(net::BitStream& stream, Create3DTextLabel& message) {
    message = Create3DTextLabel{};

    if (!stream.Read(message.labelId) || !stream.Read(message.colour) ||
        !stream.ReadBytes(&message.position, sizeof(message.position)) ||
        !stream.Read(message.drawDistance) || !stream.Read(message.testLineOfSight) ||
        !stream.Read(message.attachedPlayerId) || !stream.Read(message.attachedVehicleId)) {
        return false;
    }

    return ReadCompressedText(stream, message.text, kMaxMaterialTextLength);
}

void WriteCreate3DTextLabel(net::BitStream& stream, const Create3DTextLabel& message) {
    stream.Write(message.labelId);
    stream.Write(message.colour);
    stream.WriteBytes(&message.position, sizeof(message.position));
    stream.Write(message.drawDistance);
    stream.Write(message.testLineOfSight);
    stream.Write(message.attachedPlayerId);
    stream.Write(message.attachedVehicleId);
    WriteCompressedText(stream, message.text);
}

bool ReadRemove3DTextLabel(net::BitStream& stream, Remove3DTextLabel& message) {
    message = Remove3DTextLabel{};
    return stream.Read(message.labelId);
}

void WriteRemove3DTextLabel(net::BitStream& stream, const Remove3DTextLabel& message) {
    stream.Write(message.labelId);
}

bool ReadSetCheckpoint(net::BitStream& stream, SetCheckpoint& message) {
    message = SetCheckpoint{};
    return stream.ReadBytes(&message.position, sizeof(message.position)) &&
           stream.Read(message.size);
}

void WriteSetCheckpoint(net::BitStream& stream, const SetCheckpoint& message) {
    stream.WriteBytes(&message.position, sizeof(message.position));
    stream.Write(message.size);
}

bool ReadSetPlayerSkillLevel(net::BitStream& stream, SetPlayerSkillLevel& message) {
    message = SetPlayerSkillLevel{};
    return stream.Read(message.playerId) && stream.Read(message.skillType) &&
           stream.Read(message.level);
}

void WriteSetPlayerSkillLevel(net::BitStream& stream, const SetPlayerSkillLevel& message) {
    stream.Write(message.playerId);
    stream.Write(message.skillType);
    stream.Write(message.level);
}

bool ReadRemoveBuildingForPlayer(net::BitStream& stream, RemoveBuildingForPlayer& message) {
    message = RemoveBuildingForPlayer{};

    return stream.Read(message.modelId) &&
           stream.ReadBytes(&message.position, sizeof(message.position)) &&
           stream.Read(message.radius);
}

void WriteRemoveBuildingForPlayer(net::BitStream& stream, const RemoveBuildingForPlayer& message) {
    stream.Write(message.modelId);
    stream.WriteBytes(&message.position, sizeof(message.position));
    stream.Write(message.radius);
}

bool ReadDeathMessage(net::BitStream& stream, DeathMessage& message) {
    message = DeathMessage{};

    return stream.Read(message.killerId) && stream.Read(message.victimId) &&
           stream.Read(message.reason);
}

void WriteDeathMessage(net::BitStream& stream, const DeathMessage& message) {
    stream.Write(message.killerId);
    stream.Write(message.victimId);
    stream.Write(message.reason);
}

bool ReadSelectTextDraw(net::BitStream& stream, SelectTextDraw& message) {
    message = SelectTextDraw{};

    return stream.ReadBit(message.enabled) && stream.Read(message.hoverColour);
}

void WriteSelectTextDraw(net::BitStream& stream, const SelectTextDraw& message) {
    stream.WriteBit(message.enabled);
    stream.Write(message.hoverColour);
}

bool ReadSetPlayerMapIcon(net::BitStream& stream, SetPlayerMapIcon& message) {
    message = SetPlayerMapIcon{};

    return stream.Read(message.iconId) &&
           stream.ReadBytes(&message.position, sizeof(message.position)) &&
           stream.Read(message.markerType) && stream.Read(message.colour) &&
           stream.Read(message.style);
}

void WriteSetPlayerMapIcon(net::BitStream& stream, const SetPlayerMapIcon& message) {
    stream.Write(message.iconId);
    stream.WriteBytes(&message.position, sizeof(message.position));
    stream.Write(message.markerType);
    stream.Write(message.colour);
    stream.Write(message.style);
}

bool ReadSetVehicleNumberPlate(net::BitStream& stream, SetVehicleNumberPlate& message) {
    message = SetVehicleNumberPlate{};

    if (!stream.Read(message.vehicleId)) {
        return false;
    }

    std::uint8_t length = 0;
    if (!stream.Read(length) || length > kMaxNumberPlateLength) {
        return false;
    }

    if (length > 0) {
        message.plate.resize(length);
        if (!stream.ReadBytes(message.plate.data(), length)) {
            message = SetVehicleNumberPlate{};
            return false;
        }
    }
    return true;
}

void WriteSetVehicleNumberPlate(net::BitStream& stream, const SetVehicleNumberPlate& message) {
    const auto length =
        static_cast<std::uint8_t>(std::min(message.plate.size(), kMaxNumberPlateLength));

    stream.Write(message.vehicleId);
    stream.Write(length);
    stream.WriteBytes(message.plate.data(), length);
}

bool ReadRemoveVehicleComponent(net::BitStream& stream, RemoveVehicleComponent& message) {
    message = RemoveVehicleComponent{};
    return stream.Read(message.vehicleId) && stream.Read(message.componentId);
}

void WriteRemoveVehicleComponent(net::BitStream& stream, const RemoveVehicleComponent& message) {
    stream.Write(message.vehicleId);
    stream.Write(message.componentId);
}

bool ReadLinkVehicleToInterior(net::BitStream& stream, LinkVehicleToInterior& message) {
    message = LinkVehicleToInterior{};
    return stream.Read(message.vehicleId) && stream.Read(message.interiorId);
}

void WriteLinkVehicleToInterior(net::BitStream& stream, const LinkVehicleToInterior& message) {
    stream.Write(message.vehicleId);
    stream.Write(message.interiorId);
}

bool ReadAttachTrailerToVehicle(net::BitStream& stream, AttachTrailerToVehicle& message) {
    message = AttachTrailerToVehicle{};
    return stream.Read(message.trailerId) && stream.Read(message.vehicleId);
}

void WriteAttachTrailerToVehicle(net::BitStream& stream, const AttachTrailerToVehicle& message) {
    stream.Write(message.trailerId);
    stream.Write(message.vehicleId);
}

bool ReadDetachTrailerFromVehicle(net::BitStream& stream, DetachTrailerFromVehicle& message) {
    message = DetachTrailerFromVehicle{};
    return stream.Read(message.vehicleId);
}

void WriteDetachTrailerFromVehicle(net::BitStream& stream,
                                   const DetachTrailerFromVehicle& message) {
    stream.Write(message.vehicleId);
}

bool ReadSetPlayerWorldBounds(net::BitStream& stream, SetPlayerWorldBounds& message) {
    message = SetPlayerWorldBounds{};

    return stream.Read(message.maxX) && stream.Read(message.minX) && stream.Read(message.maxY) &&
           stream.Read(message.minY);
}

void WriteSetPlayerWorldBounds(net::BitStream& stream, const SetPlayerWorldBounds& message) {
    stream.Write(message.maxX);
    stream.Write(message.minX);
    stream.Write(message.maxY);
    stream.Write(message.minY);
}

bool ReadSpawnInfo(net::BitStream& stream, SpawnInfo& info) {
    info = SpawnInfo{};

    if (!stream.Read(info.team) || !stream.Read(info.skin) || !stream.Read(info.unused) ||
        !stream.ReadBytes(&info.position, sizeof(info.position)) || !stream.Read(info.rotation)) {
        return false;
    }

    for (std::int32_t& weapon : info.weapons) {
        if (!stream.Read(weapon)) {
            info = SpawnInfo{};
            return false;
        }
    }

    for (std::int32_t& rounds : info.ammo) {
        if (!stream.Read(rounds)) {
            info = SpawnInfo{};
            return false;
        }
    }

    return true;
}

void WriteSpawnInfo(net::BitStream& stream, const SpawnInfo& info) {
    stream.Write(info.team);
    stream.Write(info.skin);
    stream.Write(info.unused);
    stream.WriteBytes(&info.position, sizeof(info.position));
    stream.Write(info.rotation);

    for (std::int32_t weapon : info.weapons) {
        stream.Write(weapon);
    }
    for (std::int32_t rounds : info.ammo) {
        stream.Write(rounds);
    }
}

bool ReadSetPlayerName(net::BitStream& stream, SetPlayerName& message) {
    message = SetPlayerName{};

    if (!stream.Read(message.playerId)) {
        return false;
    }

    std::uint8_t length = 0;
    if (!stream.Read(length) || length > kMaxPlayerNameLength) {
        return false;
    }

    if (length > 0) {
        message.name.resize(length);
        if (!stream.ReadBytes(message.name.data(), length)) {
            message = SetPlayerName{};
            return false;
        }
    }

    return stream.Read(message.result);
}

void WriteSetPlayerName(net::BitStream& stream, const SetPlayerName& message) {
    const auto length =
        static_cast<std::uint8_t>(std::min(message.name.size(), kMaxPlayerNameLength));

    stream.Write(message.playerId);
    stream.Write(length);
    stream.WriteBytes(message.name.data(), length);
    stream.Write(message.result);
}

bool ReadSetPlayerTeam(net::BitStream& stream, SetPlayerTeam& message) {
    message = SetPlayerTeam{};
    return stream.Read(message.playerId) && stream.Read(message.team);
}

void WriteSetPlayerTeam(net::BitStream& stream, const SetPlayerTeam& message) {
    stream.Write(message.playerId);
    stream.Write(message.team);
}

bool ReadDisplayGameText(net::BitStream& stream, DisplayGameText& message) {
    message = DisplayGameText{};

    std::int32_t length = 0;
    if (!stream.Read(message.style) || !stream.Read(message.durationMs) ||
        !stream.Read(length)) {
        return false;
    }

    if (length < 1 || length > kMaxGameTextLength) {
        return false;
    }

    message.text.resize(static_cast<std::size_t>(length));
    if (!stream.ReadBytes(message.text.data(), static_cast<std::size_t>(length))) {
        message = DisplayGameText{};
        return false;
    }
    return true;
}

void WriteDisplayGameText(net::BitStream& stream, const DisplayGameText& message) {
    const auto length = static_cast<std::int32_t>(
        std::min<std::size_t>(message.text.size(), kMaxGameTextLength));

    stream.Write(message.style);
    stream.Write(message.durationMs);
    stream.Write(length);
    stream.WriteBytes(message.text.data(), static_cast<std::size_t>(length));
}

namespace {

bool ReadTextDrawText(net::BitStream& stream, std::string& text, std::uint16_t limit) {
    text.clear();

    std::uint16_t length = 0;
    if (!stream.Read(length) || length > limit) {
        return false;
    }

    if (length == 0) {
        return true;
    }

    text.resize(length);
    if (!stream.ReadBytes(text.data(), length)) {
        text.clear();
        return false;
    }
    return true;
}

void WriteTextDrawText(net::BitStream& stream, std::string_view text, std::uint16_t limit) {
    const auto length = static_cast<std::uint16_t>(std::min<std::size_t>(text.size(), limit));
    stream.Write(length);
    stream.WriteBytes(text.data(), length);
}

}

bool ReadTextDrawStyle(net::BitStream& stream, TextDrawStyle& style) {
    style = TextDrawStyle{};

    return stream.Read(style.flags) && stream.Read(style.letterWidth) &&
           stream.Read(style.letterHeight) && stream.Read(style.letterColour) &&
           stream.Read(style.lineWidth) && stream.Read(style.lineHeight) &&
           stream.Read(style.boxColour) && stream.Read(style.shadow) &&
           stream.Read(style.outline) && stream.Read(style.backgroundColour) &&
           stream.Read(style.style) && stream.Read(style.selectable) && stream.Read(style.x) &&
           stream.Read(style.y) && stream.Read(style.previewModel) &&
           stream.ReadBytes(&style.previewRotation, sizeof(style.previewRotation)) &&
           stream.Read(style.previewZoom) && stream.Read(style.previewColour1) &&
           stream.Read(style.previewColour2);
}

void WriteTextDrawStyle(net::BitStream& stream, const TextDrawStyle& style) {
    stream.Write(style.flags);
    stream.Write(style.letterWidth);
    stream.Write(style.letterHeight);
    stream.Write(style.letterColour);
    stream.Write(style.lineWidth);
    stream.Write(style.lineHeight);
    stream.Write(style.boxColour);
    stream.Write(style.shadow);
    stream.Write(style.outline);
    stream.Write(style.backgroundColour);
    stream.Write(style.style);
    stream.Write(style.selectable);
    stream.Write(style.x);
    stream.Write(style.y);
    stream.Write(style.previewModel);
    stream.WriteBytes(&style.previewRotation, sizeof(style.previewRotation));
    stream.Write(style.previewZoom);
    stream.Write(style.previewColour1);
    stream.Write(style.previewColour2);
}

bool ReadShowTextDraw(net::BitStream& stream, ShowTextDraw& message) {
    message = ShowTextDraw{};

    if (!stream.Read(message.textDrawId) || !ReadTextDrawStyle(stream, message.style)) {
        return false;
    }

    return ReadTextDrawText(stream, message.text, kMaxTextDrawTextLength - 1);
}

void WriteShowTextDraw(net::BitStream& stream, const ShowTextDraw& message) {
    stream.Write(message.textDrawId);
    WriteTextDrawStyle(stream, message.style);
    WriteTextDrawText(stream, message.text, kMaxTextDrawTextLength - 1);
}

bool ReadHideTextDraw(net::BitStream& stream, HideTextDraw& message) {
    message = HideTextDraw{};
    return stream.Read(message.textDrawId);
}

void WriteHideTextDraw(net::BitStream& stream, const HideTextDraw& message) {
    stream.Write(message.textDrawId);
}

bool ReadSetTextDrawString(net::BitStream& stream, SetTextDrawString& message) {
    message = SetTextDrawString{};

    if (!stream.Read(message.textDrawId)) {
        return false;
    }

    return ReadTextDrawText(stream, message.text, kMaxTextDrawTextLength);
}

void WriteSetTextDrawString(net::BitStream& stream, const SetTextDrawString& message) {
    stream.Write(message.textDrawId);
    WriteTextDrawText(stream, message.text, kMaxTextDrawTextLength);
}

bool ReadMoveObject(net::BitStream& stream, MoveObject& message) {
    message = MoveObject{};

    return stream.Read(message.objectId) &&
           stream.ReadBytes(&message.currentPosition, sizeof(message.currentPosition)) &&
           stream.ReadBytes(&message.targetPosition, sizeof(message.targetPosition)) &&
           stream.Read(message.speed) &&
           stream.ReadBytes(&message.targetRotation, sizeof(message.targetRotation));
}

void WriteMoveObject(net::BitStream& stream, const MoveObject& message) {
    stream.Write(message.objectId);
    stream.WriteBytes(&message.currentPosition, sizeof(message.currentPosition));
    stream.WriteBytes(&message.targetPosition, sizeof(message.targetPosition));
    stream.Write(message.speed);
    stream.WriteBytes(&message.targetRotation, sizeof(message.targetRotation));
}

bool ReadDestroyObject(net::BitStream& stream, DestroyObject& message) {
    message = DestroyObject{};
    return stream.Read(message.objectId);
}

void WriteDestroyObject(net::BitStream& stream, const DestroyObject& message) {
    stream.Write(message.objectId);
}

bool ReadTogglePlayerControllable(net::BitStream& stream, TogglePlayerControllable& message) {
    message = TogglePlayerControllable{};

    std::uint8_t flag = 0;
    if (!stream.Read(flag)) {
        return false;
    }

    message.controllable = flag != 0;
    return true;
}

void WriteTogglePlayerControllable(net::BitStream& stream,
                                   const TogglePlayerControllable& message) {
    stream.Write(static_cast<std::uint8_t>(message.controllable ? 1 : 0));
}

bool ReadPlaySound(net::BitStream& stream, PlaySound& message) {
    message = PlaySound{};
    return stream.Read(message.soundId) &&
           stream.ReadBytes(&message.position, sizeof(message.position));
}

void WritePlaySound(net::BitStream& stream, const PlaySound& message) {
    stream.Write(message.soundId);
    stream.WriteBytes(&message.position, sizeof(message.position));
}

bool ReadSetPlayerArmedWeapon(net::BitStream& stream, SetPlayerArmedWeapon& message) {
    message = SetPlayerArmedWeapon{};

    if (!stream.Read(message.weaponId)) {
        return false;
    }

    if (!IsValidWeaponId(message.weaponId)) {
        message = SetPlayerArmedWeapon{};
        return false;
    }
    return true;
}

void WriteSetPlayerArmedWeapon(net::BitStream& stream, const SetPlayerArmedWeapon& message) {
    stream.Write(message.weaponId);
}

bool ReadSetPlayerWantedLevel(net::BitStream& stream, SetPlayerWantedLevel& message) {
    message = SetPlayerWantedLevel{};
    return stream.Read(message.level);
}

void WriteSetPlayerWantedLevel(net::BitStream& stream, const SetPlayerWantedLevel& message) {
    stream.Write(message.level);
}

bool ReadCompressedText(net::BitStream& stream, std::string& text, std::size_t maxLength) {
    text.clear();

    std::uint16_t bitCount = 0;
    if (!stream.ReadCompressed(bitCount)) {
        return false;
    }

    if (stream.GetNumberOfUnreadBits() < bitCount) {
        return false;
    }

    const compression::HuffmanTree& tree = compression::HuffmanTree::Default();

    text.resize(maxLength);
    const std::size_t produced = tree.Decode(stream, bitCount, text.data(), maxLength);
    text.resize(std::min(produced, maxLength));
    return true;
}

void WriteCompressedText(net::BitStream& stream, std::string_view text) {
    const compression::HuffmanTree& tree = compression::HuffmanTree::Default();

    net::BitStream encoded;
    const int bitCount = tree.Encode(text.data(), text.size(), encoded);

    stream.WriteCompressed(static_cast<std::uint16_t>(bitCount));
    stream.CopyBitsFrom(encoded, bitCount);
}

bool ReadGivePlayerWeapon(net::BitStream& stream, GivePlayerWeapon& message) {
    message = GivePlayerWeapon{};
    return stream.Read(message.weaponId) && stream.Read(message.ammo);
}

void WriteGivePlayerWeapon(net::BitStream& stream, const GivePlayerWeapon& message) {
    stream.Write(message.weaponId);
    stream.Write(message.ammo);
}

bool ReadSetPlayerAmmo(net::BitStream& stream, SetPlayerAmmo& message) {
    message = SetPlayerAmmo{};
    return stream.Read(message.weaponSlot) && stream.Read(message.ammo);
}

void WriteSetPlayerAmmo(net::BitStream& stream, const SetPlayerAmmo& message) {
    stream.Write(message.weaponSlot);
    stream.Write(message.ammo);
}

bool ReadRemovePlayerMapIcon(net::BitStream& stream, RemovePlayerMapIcon& message) {
    message = RemovePlayerMapIcon{};
    return stream.Read(message.iconId);
}

void WriteRemovePlayerMapIcon(net::BitStream& stream, const RemovePlayerMapIcon& message) {
    stream.Write(message.iconId);
}

bool ReadTogglePlayerSpectating(net::BitStream& stream, TogglePlayerSpectating& message) {
    message = TogglePlayerSpectating{};
    return stream.Read(message.state);
}

void WriteTogglePlayerSpectating(net::BitStream& stream, const TogglePlayerSpectating& message) {
    stream.Write(message.state);
}

bool ReadSpectateTarget(net::BitStream& stream, SpectateTarget& message) {
    message = SpectateTarget{};
    return stream.Read(message.targetId) && stream.Read(message.mode);
}

void WriteSpectateTarget(net::BitStream& stream, const SpectateTarget& message) {
    stream.Write(message.targetId);
    stream.Write(message.mode);
}

std::uint8_t CameraModeForSpectate(std::uint8_t mode, bool spectatingVehicle) {
    if (mode == 2) {
        return 15;
    }
    if (mode == 3) {
        return 14;
    }
    return spectatingVehicle ? 3 : 4;
}

bool ReadCreateObject(net::BitStream& stream, CreateObject& message) {
    message = CreateObject{};

    std::uint8_t noCameraCollision = 0;
    if (!stream.Read(message.objectId) || !stream.Read(message.modelId) ||
        !stream.ReadBytes(&message.position, sizeof(message.position)) ||
        !stream.ReadBytes(&message.rotation, sizeof(message.rotation)) ||
        !stream.Read(message.drawDistance) || !stream.Read(noCameraCollision) ||
        !stream.Read(message.attachedVehicleId) || !stream.Read(message.attachedObjectId)) {
        return false;
    }
    message.noCameraCollision = noCameraCollision != 0;

    if (message.IsAttached()) {
        std::uint8_t syncRotation = 0;
        if (!stream.ReadBytes(&message.attachment.offset, sizeof(message.attachment.offset)) ||
            !stream.ReadBytes(&message.attachment.rotation, sizeof(message.attachment.rotation)) ||
            !stream.Read(syncRotation)) {
            return false;
        }
        message.attachment.syncRotation = syncRotation != 0;
    }

    return stream.Read(message.materialCount);
}

void WriteCreateObject(net::BitStream& stream, const CreateObject& message) {
    stream.Write(message.objectId);
    stream.Write(message.modelId);
    stream.WriteBytes(&message.position, sizeof(message.position));
    stream.WriteBytes(&message.rotation, sizeof(message.rotation));
    stream.Write(message.drawDistance);
    stream.Write(static_cast<std::uint8_t>(message.noCameraCollision ? 1 : 0));
    stream.Write(message.attachedVehicleId);
    stream.Write(message.attachedObjectId);

    if (message.IsAttached()) {
        stream.WriteBytes(&message.attachment.offset, sizeof(message.attachment.offset));
        stream.WriteBytes(&message.attachment.rotation, sizeof(message.attachment.rotation));
        stream.Write(static_cast<std::uint8_t>(message.attachment.syncRotation ? 1 : 0));
    }

    stream.Write(message.materialCount);
}

bool ObjectMaterial::IsValid() const {
    if (const auto* texture = std::get_if<ObjectMaterialTexture>(&content)) {
        return type == ObjectMaterialType::Texture && texture->IsValid();
    }
    if (const auto* text = std::get_if<ObjectMaterialText>(&content)) {
        return type == ObjectMaterialType::Text && text->IsValid();
    }
    return false;
}

bool ReadObjectMaterial(net::BitStream& stream, ObjectMaterial& material) {
    material = ObjectMaterial{};

    std::uint8_t type = 0;
    if (!stream.Read(type)) {
        return false;
    }

    if (type == static_cast<std::uint8_t>(ObjectMaterialType::Texture)) {
        material.type = ObjectMaterialType::Texture;

        ObjectMaterialTexture texture;
        if (!stream.Read(texture.materialIndex) || !stream.Read(texture.modelId) ||
            !ReadString8(stream, texture.txdName) || !ReadString8(stream, texture.textureName) ||
            !stream.Read(texture.colour)) {
            return false;
        }

        material.content = texture;
        return true;
    }

    if (type == static_cast<std::uint8_t>(ObjectMaterialType::Text)) {
        material.type = ObjectMaterialType::Text;

        ObjectMaterialText text;
        std::uint8_t bold = 0;
        if (!stream.Read(text.materialIndex) || !stream.Read(text.materialSize) ||
            !ReadString8(stream, text.fontName) || !stream.Read(text.fontSize) ||
            !stream.Read(bold) || !stream.Read(text.fontColour) ||
            !stream.Read(text.backgroundColour) || !stream.Read(text.alignment) ||
            !ReadCompressedText(stream, text.text, kMaxMaterialTextLength)) {
            return false;
        }
        text.bold = bold != 0;

        material.content = text;
        return true;
    }

    return false;
}

void WriteObjectMaterial(net::BitStream& stream, const ObjectMaterial& material) {
    stream.Write(static_cast<std::uint8_t>(material.type));

    if (const auto* texture = std::get_if<ObjectMaterialTexture>(&material.content)) {
        stream.Write(texture->materialIndex);
        stream.Write(texture->modelId);
        WriteString8(stream, texture->txdName);
        WriteString8(stream, texture->textureName);
        stream.Write(texture->colour);
        return;
    }

    if (const auto* text = std::get_if<ObjectMaterialText>(&material.content)) {
        stream.Write(text->materialIndex);
        stream.Write(text->materialSize);
        WriteString8(stream, text->fontName);
        stream.Write(text->fontSize);
        stream.Write(static_cast<std::uint8_t>(text->bold ? 1 : 0));
        stream.Write(text->fontColour);
        stream.Write(text->backgroundColour);
        stream.Write(text->alignment);
        WriteCompressedText(stream, text->text);
    }
}

bool ReadSetObjectMaterial(net::BitStream& stream, SetObjectMaterial& message) {
    message = SetObjectMaterial{};
    return stream.Read(message.objectId) && ReadObjectMaterial(stream, message.material);
}

void WriteSetObjectMaterial(net::BitStream& stream, const SetObjectMaterial& message) {
    stream.Write(message.objectId);
    WriteObjectMaterial(stream, message.material);
}

bool ReadObjectMaterials(net::BitStream& stream, std::uint8_t count,
                         std::vector<ObjectMaterial>& materials) {
    materials.clear();
    materials.reserve(count);

    for (std::uint8_t i = 0; i < count; ++i) {
        ObjectMaterial material;
        if (!ReadObjectMaterial(stream, material)) {
            materials.clear();
            return false;
        }
        materials.push_back(std::move(material));
    }

    return true;
}

}
