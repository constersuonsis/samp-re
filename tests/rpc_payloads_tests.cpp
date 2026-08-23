#include "samp/protocol/pool_limits.h"
#include "samp/protocol/rpc_payloads.h"

#include <array>
#include <cstdio>
#include <string>
#include <variant>
#include <vector>

namespace {

int g_failures = 0;

void Check(bool condition, const char* what) {
    if (!condition) {
        std::printf("FAIL: %s\n", what);
        ++g_failures;
    }
}

void TestChatRoundTrip() {
    const samp::protocol::ChatMessage sent{7, "hello world"};

    samp::net::BitStream stream;
    samp::protocol::WriteChatMessage(stream, sent);

    Check(stream.GetNumberOfBitsUsed() == (2 + 1 + 11) * 8, "chat message costs no padding");

    samp::protocol::ChatMessage received;
    Check(samp::protocol::ReadChatMessage(stream, received), "chat message reads back");
    Check(received.playerId == 7, "chat player id round-trips");
    Check(received.text == "hello world", "chat text round-trips");
}

void TestEmptyStringRoundTrip() {
    const samp::protocol::ChatMessage sent{1, ""};

    samp::net::BitStream stream;
    samp::protocol::WriteChatMessage(stream, sent);

    samp::protocol::ChatMessage received{99, "stale"};
    Check(samp::protocol::ReadChatMessage(stream, received), "empty text reads back");
    Check(received.text.empty(), "empty text stays empty");
}

void TestClientMessageRoundTrip() {
    const samp::protocol::ClientMessage sent{0xFF0000FF, "server says hi"};

    samp::net::BitStream stream;
    samp::protocol::WriteClientMessage(stream, sent);

    samp::protocol::ClientMessage received;
    Check(samp::protocol::ReadClientMessage(stream, received), "client message reads back");
    Check(received.colour == 0xFF0000FF, "colour round-trips");
    Check(received.text == "server says hi", "client message text round-trips");
}

void TestOversizedLengthIsRejected() {

    samp::net::BitStream stream;
    stream.Write<std::uint32_t>(0xFFFFFF);
    stream.Write<std::uint32_t>(0xDEADBEEF);

    samp::protocol::ClientMessage received{1, "stale"};
    Check(!samp::protocol::ReadClientMessage(stream, received),
          "an oversized length is rejected rather than trusted");
    Check(received.text.empty(), "a rejected message leaves no stale text behind");
}

void TestTruncatedStringIsRejected() {

    samp::net::BitStream stream;
    stream.Write<std::uint16_t>(3);
    stream.Write<std::uint8_t>(8);
    stream.WriteBytes("abc", 3);

    samp::protocol::ChatMessage received;
    Check(!samp::protocol::ReadChatMessage(stream, received),
          "a truncated string is rejected instead of reading past the end");
}

void TestLongStringIsClamped() {
    const std::string tooLong(400, 'x');

    samp::net::BitStream stream;
    samp::protocol::WriteChatMessage(stream, {2, tooLong});

    samp::protocol::ChatMessage received;
    Check(samp::protocol::ReadChatMessage(stream, received), "clamped message reads back");
    Check(received.text.size() == samp::protocol::kMaxStringLength,
          "a string longer than the length field is clamped, not truncated to nonsense");
}

void TestDialogHeaderRoundTrip() {
    const samp::protocol::DialogHeader sent{1337, 2, "Title", "Ok", "Cancel"};

    samp::net::BitStream stream;
    samp::protocol::WriteDialogHeader(stream, sent);

    samp::protocol::DialogHeader received;
    Check(samp::protocol::ReadDialogHeader(stream, received), "dialog header reads back");
    Check(received.dialogId == 1337, "dialog id round-trips");
    Check(received.style == 2, "dialog style round-trips");
    Check(received.title == "Title", "dialog title round-trips");
    Check(received.firstButton == "Ok", "first button round-trips");
    Check(received.secondButton == "Cancel", "second button round-trips");
}

void TestVehicleEntryRoundTrip() {
    samp::net::BitStream stream;
    samp::protocol::WriteEnterVehicle(stream, {12, 340, true});
    Check(stream.GetNumberOfBitsUsed() == 5 * 8, "vehicle entry is five bytes");

    samp::protocol::EnterVehicle entered;
    Check(samp::protocol::ReadEnterVehicle(stream, entered), "vehicle entry reads back");
    Check(entered.playerId == 12, "entering player round-trips");
    Check(entered.vehicleId == 340, "entered vehicle round-trips");
    Check(entered.asPassenger, "passenger flag round-trips");
}

void TestVehicleExitRoundTrip() {
    samp::net::BitStream stream;
    samp::protocol::WriteExitVehicle(stream, {12, 340});
    Check(stream.GetNumberOfBitsUsed() == 4 * 8, "vehicle exit is four bytes");

    samp::protocol::ExitVehicle exited;
    Check(samp::protocol::ReadExitVehicle(stream, exited), "vehicle exit reads back");
    Check(exited.playerId == 12 && exited.vehicleId == 340, "vehicle exit round-trips");
}

void TestDamageStatusRoundTrip() {
    const samp::protocol::VehicleDamageStatus sent{99, 0x12345678, 0xABCDEF01, 0x0F, 0x0B};

    samp::net::BitStream stream;
    samp::protocol::WriteVehicleDamageStatus(stream, sent);
    Check(stream.GetNumberOfBitsUsed() == (2 + 4 + 4 + 1 + 1) * 8, "damage status is twelve bytes");

    samp::protocol::VehicleDamageStatus received;
    Check(samp::protocol::ReadVehicleDamageStatus(stream, received), "damage status reads back");
    Check(received.vehicleId == 99, "damaged vehicle round-trips");
    Check(received.panelStatus == 0x12345678, "panel status round-trips");
    Check(received.doorStatus == 0xABCDEF01, "door status round-trips");
    Check(received.lightStatus == 0x0F, "light status round-trips");
    Check(received.tyreStatus == 0x0B, "tyre status round-trips");
}

void TestPickupRoundTrip() {
    const samp::protocol::CreatePickup sent{17, 1274, 2, {100.5f, -200.25f, 10.0f}};

    samp::net::BitStream stream;
    samp::protocol::WriteCreatePickup(stream, sent);
    Check(stream.GetNumberOfBitsUsed() == 24 * 8, "pickup creation is twenty-four bytes");

    samp::protocol::CreatePickup received;
    Check(samp::protocol::ReadCreatePickup(stream, received), "pickup creation reads back");
    Check(received.slot == 17, "pickup slot round-trips");
    Check(received.model == 1274, "pickup model round-trips");
    Check(received.type == 2, "pickup type round-trips");
    Check(received.position.x == 100.5f && received.position.z == 10.0f,
          "pickup position round-trips");
}

void TestPickupSlotIsBoundsChecked() {

    samp::net::BitStream stream;
    stream.Write<std::uint32_t>(samp::protocol::kPickupPoolSize);
    stream.Write<std::int32_t>(1274);
    stream.Write<std::int32_t>(2);
    stream.WriteBytes("\0\0\0\0\0\0\0\0\0\0\0\0", 12);

    samp::protocol::CreatePickup received;
    Check(!samp::protocol::ReadCreatePickup(stream, received),
          "a pickup slot past the end of the pool is rejected");
}

void TestSingleByteMessages() {
    samp::net::BitStream weatherStream;
    samp::protocol::WriteSetWeather(weatherStream, {12});
    Check(weatherStream.GetNumberOfBitsUsed() == 8, "weather is a single byte");

    samp::protocol::SetWeather weather;
    Check(samp::protocol::ReadSetWeather(weatherStream, weather) && weather.weatherId == 12,
          "weather round-trips");

    samp::net::BitStream timeStream;
    samp::protocol::WriteSetWorldTime(timeStream, {23});
    samp::protocol::SetWorldTime time;
    Check(samp::protocol::ReadSetWorldTime(timeStream, time) && time.hour == 23,
          "world time round-trips");
}

void TestPlayerStateMessages() {
    samp::net::BitStream positionStream;
    samp::protocol::WriteSetPlayerPosition(positionStream, {{1.5f, 2.5f, 3.5f}});
    Check(positionStream.GetNumberOfBitsUsed() == 12 * 8, "position message is twelve bytes");

    samp::protocol::SetPlayerPosition position;
    Check(samp::protocol::ReadSetPlayerPosition(positionStream, position) &&
              position.position.y == 2.5f,
          "player position round-trips");

    samp::net::BitStream healthStream;
    samp::protocol::WriteSetPlayerHealth(healthStream, {87.5f});
    samp::protocol::SetPlayerHealth health;
    Check(samp::protocol::ReadSetPlayerHealth(healthStream, health) && health.health == 87.5f,
          "player health keeps its fractional part");

    samp::net::BitStream armourStream;
    samp::protocol::WriteSetPlayerArmour(armourStream, {33.25f});
    samp::protocol::SetPlayerArmour armour;
    Check(samp::protocol::ReadSetPlayerArmour(armourStream, armour) && armour.armour == 33.25f,
          "player armour round-trips");

    samp::net::BitStream colourStream;
    samp::protocol::WriteSetPlayerColour(colourStream, {5, 0xAABBCCDD});
    Check(colourStream.GetNumberOfBitsUsed() == 6 * 8, "colour message is six bytes");

    samp::protocol::SetPlayerColour colour;
    Check(samp::protocol::ReadSetPlayerColour(colourStream, colour) && colour.playerId == 5 &&
              colour.colour == 0xAABBCCDD,
          "player colour round-trips");
}

void TestWorldStateMessages() {
    samp::net::BitStream gravityStream;
    samp::protocol::WriteSetGravity(gravityStream, {0.008f});
    samp::protocol::SetGravity gravity;
    Check(samp::protocol::ReadSetGravity(gravityStream, gravity) && gravity.gravity == 0.008f,
          "gravity round-trips");

    samp::net::BitStream vehicleStream;
    samp::protocol::WriteSetVehicleHealth(vehicleStream, {300, 750.0f});
    Check(vehicleStream.GetNumberOfBitsUsed() == 6 * 8, "vehicle health message is six bytes");

    samp::protocol::SetVehicleHealth vehicleHealth;
    Check(samp::protocol::ReadSetVehicleHealth(vehicleStream, vehicleHealth) &&
              vehicleHealth.vehicleId == 300 && vehicleHealth.health == 750.0f,
          "vehicle health round-trips");
}

void TestCameraMessages() {
    samp::net::BitStream posStream;
    samp::protocol::WriteSetPlayerCameraPosition(posStream, {{5.0f, 6.0f, 7.0f}});
    samp::protocol::SetPlayerCameraPosition cameraPos;
    Check(samp::protocol::ReadSetPlayerCameraPosition(posStream, cameraPos) &&
              cameraPos.position.z == 7.0f,
          "camera position round-trips");

    samp::net::BitStream lookStream;
    samp::protocol::WriteSetPlayerCameraLookAt(
        lookStream, {{1.0f, 2.0f, 3.0f}, samp::protocol::CameraCutType::Move});
    Check(lookStream.GetNumberOfBitsUsed() == 13 * 8, "camera look-at is thirteen bytes");

    samp::protocol::SetPlayerCameraLookAt lookAt;
    Check(samp::protocol::ReadSetPlayerCameraLookAt(lookStream, lookAt), "look-at reads back");
    Check(lookAt.target.x == 1.0f, "look-at target round-trips");
    Check(lookAt.cutType == samp::protocol::CameraCutType::Move, "cut type round-trips");
}

void TestUnknownCutTypeFallsBack() {

    samp::net::BitStream stream;
    stream.WriteBytes("\0\0\0\0\0\0\0\0\0\0\0\0", 12);
    stream.Write<std::uint8_t>(99);

    samp::protocol::SetPlayerCameraLookAt lookAt;
    Check(samp::protocol::ReadSetPlayerCameraLookAt(stream, lookAt), "odd cut type still reads");
    Check(lookAt.cutType == samp::protocol::CameraCutType::Cut,
          "an unknown cut type falls back instead of being passed through");
}

void TestVehiclePlacementMessages() {
    samp::net::BitStream posStream;
    samp::protocol::WriteSetVehiclePosition(posStream, {88, {10.0f, 20.0f, 30.0f}});
    Check(posStream.GetNumberOfBitsUsed() == 14 * 8, "vehicle placement is fourteen bytes");

    samp::protocol::SetVehiclePosition vehiclePos;
    Check(samp::protocol::ReadSetVehiclePosition(posStream, vehiclePos) &&
              vehiclePos.vehicleId == 88 && vehiclePos.position.y == 20.0f,
          "vehicle position round-trips");

    samp::net::BitStream angleStream;
    samp::protocol::WriteSetVehicleZAngle(angleStream, {88, 180.5f});
    samp::protocol::SetVehicleZAngle angle;
    Check(samp::protocol::ReadSetVehicleZAngle(angleStream, angle) && angle.angle == 180.5f,
          "vehicle angle round-trips");
}

void TestPoolLimits() {
    using namespace samp::protocol;

    Check(IsValidPlayerId(kMaxPlayerId), "the top player id is inside the pool");
    Check(!IsValidPlayerId(kMaxPlayerId + 1), "one past the top player id is outside");
    Check(IsValidObjectId(kMaxObjectId), "the top object id is inside the pool");
    Check(!IsValidVehicleId(kVehiclePoolSize), "the vehicle pool size is not a valid id");
    Check(IsValidVehicleId(kVehiclePoolSize - 1), "the last vehicle slot is valid");

    Check(kActorPoolSize == kMaxObjectId, "actors and objects share a limit value");
    Check(IsValidObjectId(kMaxObjectId), "the object limit is inclusive");
    Check(!IsValidActorId(kActorPoolSize), "the actor limit is not");

    Check(!IsValid3DTextLabelId(k3DTextLabelPoolSize), "the label pool excludes its size");
    Check(!IsValidTextDrawId(kTextDrawPoolSize), "the textdraw pool excludes its size");
    Check(!IsValidPickupSlot(kPickupPoolSize), "the pickup pool excludes its size");
}

void TestObjectAndPlayerToggles() {
    samp::net::BitStream objectStream;
    samp::protocol::WriteDestroyObject(objectStream, {512});
    samp::protocol::DestroyObject destroyed;
    Check(samp::protocol::ReadDestroyObject(objectStream, destroyed) && destroyed.objectId == 512,
          "object destruction round-trips");

    samp::net::BitStream toggleStream;
    samp::protocol::WriteTogglePlayerControllable(toggleStream, {true});
    Check(toggleStream.GetNumberOfBitsUsed() == 8, "controllable toggle is a single byte");

    samp::protocol::TogglePlayerControllable toggle;
    Check(samp::protocol::ReadTogglePlayerControllable(toggleStream, toggle) && toggle.controllable,
          "controllable toggle round-trips");

    samp::net::BitStream wantedStream;
    samp::protocol::WriteSetPlayerWantedLevel(wantedStream, {5});
    samp::protocol::SetPlayerWantedLevel wanted;
    Check(samp::protocol::ReadSetPlayerWantedLevel(wantedStream, wanted) && wanted.level == 5,
          "wanted level round-trips");
}

void TestPlaySoundRoundTrip() {
    samp::net::BitStream stream;
    samp::protocol::WritePlaySound(stream, {1058, {1.0f, 2.0f, 3.0f}});
    Check(stream.GetNumberOfBitsUsed() == 16 * 8, "sound message is sixteen bytes");

    samp::protocol::PlaySound sound;
    Check(samp::protocol::ReadPlaySound(stream, sound), "sound message reads back");
    Check(sound.soundId == 1058, "sound id round-trips");
    Check(sound.position.z == 3.0f, "sound position round-trips");
}

void TestArmedWeaponIsValidated() {
    samp::net::BitStream valid;
    samp::protocol::WriteSetPlayerArmedWeapon(valid, {samp::protocol::kMaxWeaponId});
    samp::protocol::SetPlayerArmedWeapon weapon;
    Check(samp::protocol::ReadSetPlayerArmedWeapon(valid, weapon) &&
              weapon.weaponId == samp::protocol::kMaxWeaponId,
          "the highest weapon id is accepted");

    samp::net::BitStream invalid;
    invalid.Write<std::uint32_t>(samp::protocol::kMaxWeaponId + 1);
    Check(!samp::protocol::ReadSetPlayerArmedWeapon(invalid, weapon),
          "a weapon id past the tables is rejected");
}

void TestCompressedTextRoundTrip() {
    const std::string body =
        "Welcome to the server.\nPick an option below to continue, or press escape "
        "to close this window.";

    samp::net::BitStream stream;
    samp::protocol::WriteCompressedText(stream, body);

    Check(stream.GetNumberOfBitsUsed() < static_cast<int>(body.size()) * 8,
          "compressed text is smaller than the plain bytes");

    std::string decoded;
    Check(samp::protocol::ReadCompressedText(stream, decoded), "compressed text reads back");
    Check(decoded == body, "compressed text round-trips exactly");
    Check(stream.GetNumberOfUnreadBits() == 0, "the whole compressed run is consumed");
}

void TestDialogWithCompressedBody() {

    const samp::protocol::DialogHeader header{7, 2, "Shop", "Buy", "Leave"};
    const std::string body = "Item one\nItem two\nItem three";

    samp::net::BitStream stream;
    samp::protocol::WriteDialogHeader(stream, header);
    samp::protocol::WriteCompressedText(stream, body);

    samp::protocol::DialogHeader readHeader;
    std::string readBody;
    Check(samp::protocol::ReadDialogHeader(stream, readHeader), "dialog header reads back");
    Check(samp::protocol::ReadCompressedText(stream, readBody), "dialog body reads back");
    Check(readHeader.title == "Shop", "header survives ahead of the body");
    Check(readBody == body, "body survives after the header");
}

void TestTruncatedCompressedTextIsRejected() {
    samp::net::BitStream stream;
    stream.WriteCompressed<std::uint16_t>(500);
    stream.WriteBit(true);

    std::string decoded = "stale";
    Check(!samp::protocol::ReadCompressedText(stream, decoded),
          "a run longer than the packet is rejected");
    Check(decoded.empty(), "a rejected body leaves nothing behind");
}

void TestWeaponMessages() {
    samp::net::BitStream giveStream;
    samp::protocol::WriteGivePlayerWeapon(giveStream, {24, 150});
    Check(giveStream.GetNumberOfBitsUsed() == 8 * 8, "giving a weapon costs eight bytes");

    samp::protocol::GivePlayerWeapon given;
    Check(samp::protocol::ReadGivePlayerWeapon(giveStream, given) && given.weaponId == 24 &&
              given.ammo == 150,
          "weapon and ammunition round-trip");

    samp::net::BitStream ammoStream;
    samp::protocol::WriteSetPlayerAmmo(ammoStream, {3, 9999});
    Check(ammoStream.GetNumberOfBitsUsed() == 3 * 8, "setting ammunition costs three bytes");

    samp::protocol::SetPlayerAmmo ammo;
    Check(samp::protocol::ReadSetPlayerAmmo(ammoStream, ammo) && ammo.weaponSlot == 3 &&
              ammo.ammo == 9999,
          "weapon slot and ammunition round-trip");
}

void TestSpectateMessages() {
    samp::net::BitStream stream;
    samp::protocol::WriteSpectateTarget(stream, {77, 2});

    samp::protocol::SpectateTarget target;
    Check(samp::protocol::ReadSpectateTarget(stream, target) && target.targetId == 77 &&
              target.mode == 2,
          "spectate target round-trips");

    samp::net::BitStream toggleStream;
    samp::protocol::WriteTogglePlayerSpectating(toggleStream, {1});
    samp::protocol::TogglePlayerSpectating toggle;
    Check(samp::protocol::ReadTogglePlayerSpectating(toggleStream, toggle) && toggle.state == 1,
          "spectating toggle round-trips");
}

void TestSpectateFallbackDiffersByTarget() {
    using samp::protocol::CameraModeForSpectate;

    Check(CameraModeForSpectate(2, false) == CameraModeForSpectate(2, true),
          "mode two maps the same either way");
    Check(CameraModeForSpectate(3, false) == CameraModeForSpectate(3, true),
          "mode three maps the same either way");

    Check(CameraModeForSpectate(0, false) == 4, "watching a player falls back to one mode");
    Check(CameraModeForSpectate(0, true) == 3, "watching a vehicle falls back to another");
}

samp::protocol::CreateObject MakeObject() {
    samp::protocol::CreateObject object;
    object.objectId = 300;
    object.modelId = 19300;
    object.position = {100.0f, 200.0f, 15.5f};
    object.rotation = {0.0f, 0.0f, 90.0f};
    object.drawDistance = 300.0f;
    object.noCameraCollision = true;
    return object;
}

void TestFreeStandingObject() {
    const samp::protocol::CreateObject sent = MakeObject();

    samp::net::BitStream stream;
    samp::protocol::WriteCreateObject(stream, sent);

    Check(stream.GetNumberOfBitsUsed() == 40 * 8, "an unattached object is forty bytes");

    samp::protocol::CreateObject received;
    Check(samp::protocol::ReadCreateObject(stream, received), "object reads back");
    Check(received.objectId == 300 && received.modelId == 19300, "identity round-trips");
    Check(received.position.z == 15.5f && received.rotation.z == 90.0f, "placement round-trips");
    Check(received.drawDistance == 300.0f, "draw distance round-trips");
    Check(received.noCameraCollision, "camera collision flag round-trips");
    Check(!received.IsAttached(), "an unattached object stays unattached");
}

void TestObjectAttachedToVehicle() {
    samp::protocol::CreateObject sent = MakeObject();
    sent.attachedVehicleId = 55;
    sent.attachment.offset = {0.0f, 1.0f, 2.0f};
    sent.attachment.rotation = {0.0f, 0.0f, 45.0f};
    sent.attachment.syncRotation = true;

    samp::net::BitStream stream;
    samp::protocol::WriteCreateObject(stream, sent);
    Check(stream.GetNumberOfBitsUsed() == 65 * 8, "an attached object carries 25 more bytes");

    samp::protocol::CreateObject received;
    Check(samp::protocol::ReadCreateObject(stream, received), "attached object reads back");
    Check(received.IsAttached(), "attachment survives");
    Check(received.attachedVehicleId == 55, "vehicle attachment round-trips");
    Check(received.attachedObjectId == samp::protocol::kNoAttachment,
          "the unused attachment slot stays empty");
    Check(received.attachment.offset.z == 2.0f, "attachment offset round-trips");
    Check(received.attachment.rotation.z == 45.0f, "attachment rotation round-trips");
}

void TestObjectAttachedToObject() {
    samp::protocol::CreateObject sent = MakeObject();
    sent.attachedObjectId = 7;
    sent.attachment.offset = {1.0f, 0.0f, 0.0f};

    samp::net::BitStream stream;
    samp::protocol::WriteCreateObject(stream, sent);

    samp::protocol::CreateObject received;
    Check(samp::protocol::ReadCreateObject(stream, received), "object attachment reads back");
    Check(received.attachedObjectId == 7, "object attachment round-trips");
    Check(received.attachedVehicleId == samp::protocol::kNoAttachment,
          "the vehicle slot stays empty");
}

void TestObjectMaterialCountIsCarried() {
    samp::protocol::CreateObject sent = MakeObject();
    sent.materialCount = 3;

    samp::net::BitStream stream;
    samp::protocol::WriteCreateObject(stream, sent);

    samp::protocol::CreateObject received;
    Check(samp::protocol::ReadCreateObject(stream, received), "object with materials reads back");
    Check(received.materialCount == 3, "the material count survives");
    Check(stream.GetNumberOfUnreadBits() == 0, "nothing is left over before the material list");
}

samp::protocol::ObjectMaterial MakeTextureMaterial() {
    samp::protocol::ObjectMaterialTexture texture;
    texture.materialIndex = 1;
    texture.modelId = 18646;
    texture.txdName = "vgnhseblokb01";
    texture.textureName = "vgnhseblok1";
    texture.colour = 0xFFAABBCC;

    samp::protocol::ObjectMaterial material;
    material.type = samp::protocol::ObjectMaterialType::Texture;
    material.content = texture;
    return material;
}

samp::protocol::ObjectMaterial MakeTextMaterial() {
    samp::protocol::ObjectMaterialText text;
    text.materialIndex = 0;
    text.materialSize = 90;
    text.fontName = "Arial";
    text.fontSize = 24;
    text.bold = true;
    text.fontColour = 0xFFFFFFFF;
    text.backgroundColour = 0xFF000000;
    text.alignment = 1;
    text.text = "Open for business";

    samp::protocol::ObjectMaterial material;
    material.type = samp::protocol::ObjectMaterialType::Text;
    material.content = text;
    return material;
}

void TestTextureMaterialRoundTrip() {
    const samp::protocol::ObjectMaterial sent = MakeTextureMaterial();

    samp::net::BitStream stream;
    samp::protocol::WriteObjectMaterial(stream, sent);

    samp::protocol::ObjectMaterial received;
    Check(samp::protocol::ReadObjectMaterial(stream, received), "texture material reads back");
    Check(received.type == samp::protocol::ObjectMaterialType::Texture, "type round-trips");
    Check(received.IsValid(), "a well-formed texture material is valid");

    const auto& texture = std::get<samp::protocol::ObjectMaterialTexture>(received.content);
    Check(texture.txdName == "vgnhseblokb01", "archive name round-trips");
    Check(texture.textureName == "vgnhseblok1", "texture name round-trips");
    Check(texture.colour == 0xFFAABBCC, "material colour round-trips");
}

void TestTextMaterialRoundTrip() {
    const samp::protocol::ObjectMaterial sent = MakeTextMaterial();

    samp::net::BitStream stream;
    samp::protocol::WriteObjectMaterial(stream, sent);

    samp::protocol::ObjectMaterial received;
    Check(samp::protocol::ReadObjectMaterial(stream, received), "text material reads back");
    Check(received.IsValid(), "a well-formed text material is valid");

    const auto& text = std::get<samp::protocol::ObjectMaterialText>(received.content);
    Check(text.fontName == "Arial", "font name round-trips");
    Check(text.bold, "bold flag round-trips");
    Check(text.backgroundColour == 0xFF000000, "background colour round-trips");
    Check(text.text == "Open for business", "compressed material text round-trips");
}

void TestMaterialModelRulesDiffer() {
    using samp::protocol::MaterialModelRule;
    using samp::protocol::NormaliseMaterialModelId;

    const std::uint16_t pastRange = samp::protocol::kMaxMaterialModelId + 1;

    Check(NormaliseMaterialModelId(pastRange, MaterialModelRule::RejectAboveRange) == -1,
          "creating an object treats a model past the range as absent");
    Check(NormaliseMaterialModelId(pastRange, MaterialModelRule::RejectSentinelOnly) == pastRange,
          "updating a material keeps the very same value");

    Check(NormaliseMaterialModelId(samp::protocol::kNoMaterialModel,
                                   MaterialModelRule::RejectSentinelOnly) == -1,
          "the sentinel is absent under the update rule");
    Check(NormaliseMaterialModelId(samp::protocol::kMaxMaterialModelId,
                                   MaterialModelRule::RejectAboveRange) ==
              samp::protocol::kMaxMaterialModelId,
          "the last in-range model is kept either way");
}

void TestSetObjectMaterialReusesTheEntryFormat() {
    samp::protocol::SetObjectMaterial sent;
    sent.objectId = 321;
    sent.material = MakeTextMaterial();

    samp::net::BitStream stream;
    samp::protocol::WriteSetObjectMaterial(stream, sent);

    samp::protocol::SetObjectMaterial received;
    Check(samp::protocol::ReadSetObjectMaterial(stream, received),
          "a standalone material update reads back");
    Check(received.objectId == 321, "the target object round-trips");
    Check(received.material.type == samp::protocol::ObjectMaterialType::Text,
          "the entry kind round-trips");
    Check(std::get<samp::protocol::ObjectMaterialText>(received.material.content).text ==
              "Open for business",
          "the entry body round-trips through the shared format");
}

void TestOverlongNameStaysReadableButInvalid() {
    samp::protocol::ObjectMaterialTexture texture;
    texture.materialIndex = 0;
    texture.modelId = 1;
    texture.txdName = std::string(200, 'x');
    texture.textureName = "fine";

    samp::protocol::ObjectMaterial material;
    material.type = samp::protocol::ObjectMaterialType::Texture;
    material.content = texture;

    samp::net::BitStream stream;
    samp::protocol::WriteObjectMaterial(stream, material);

    samp::protocol::WriteObjectMaterial(stream, MakeTextureMaterial());

    samp::protocol::ObjectMaterial first;
    samp::protocol::ObjectMaterial second;
    Check(samp::protocol::ReadObjectMaterial(stream, first), "an overlong name still reads");
    Check(!first.IsValid(), "an overlong name marks the entry invalid");
    Check(samp::protocol::ReadObjectMaterial(stream, second), "the next entry is still aligned");
    Check(second.IsValid(), "the entry after a bad one is intact");
}

void TestMaterialListMatchesCount() {
    samp::net::BitStream stream;
    samp::protocol::WriteObjectMaterial(stream, MakeTextureMaterial());
    samp::protocol::WriteObjectMaterial(stream, MakeTextMaterial());
    samp::protocol::WriteObjectMaterial(stream, MakeTextureMaterial());

    std::vector<samp::protocol::ObjectMaterial> materials;
    Check(samp::protocol::ReadObjectMaterials(stream, 3, materials), "the list reads back");
    Check(materials.size() == 3, "every entry is present");
    Check(materials[1].type == samp::protocol::ObjectMaterialType::Text,
          "mixed entry kinds keep their order");
    Check(stream.GetNumberOfUnreadBits() == 0, "the list consumes exactly its entries");
}

void TestUnknownMaterialKindIsRejected() {
    samp::net::BitStream stream;
    stream.Write<std::uint8_t>(9);

    samp::protocol::ObjectMaterial material;
    Check(!samp::protocol::ReadObjectMaterial(stream, material),
          "an unknown entry kind is rejected because its length is unknowable");
}

void TestMoveObjectRoundTrip() {
    samp::protocol::MoveObject sent;
    sent.objectId = 42;
    sent.currentPosition = {1.0f, 2.0f, 3.0f};
    sent.targetPosition = {10.0f, 20.0f, 30.0f};
    sent.speed = 2.5f;
    sent.targetRotation = {0.0f, 0.0f, 180.0f};

    samp::net::BitStream stream;
    samp::protocol::WriteMoveObject(stream, sent);
    Check(stream.GetNumberOfBitsUsed() == 42 * 8, "an object move is forty-two bytes");

    samp::protocol::MoveObject received;
    Check(samp::protocol::ReadMoveObject(stream, received), "object move reads back");
    Check(received.objectId == 42, "object id round-trips");
    Check(received.targetPosition.y == 20.0f, "target position round-trips");
    Check(received.speed == 2.5f, "speed round-trips");
    Check(received.targetRotation.z == 180.0f, "target rotation round-trips");

    Check(received.currentPosition.x == 1.0f && received.currentPosition.z == 3.0f,
          "the unused current position is still parsed correctly");
}

void TestTextDrawRoundTrip() {
    samp::protocol::ShowTextDraw sent;
    sent.textDrawId = 12;
    sent.style.flags = 0x0D;
    sent.style.letterWidth = 0.5f;
    sent.style.letterHeight = 1.5f;
    sent.style.letterColour = 0xFFFFFFFF;
    sent.style.boxColour = 0x80000000;
    sent.style.shadow = 1;
    sent.style.outline = 2;
    sent.style.style = 3;
    sent.style.x = 320.0f;
    sent.style.y = 240.0f;
    sent.text = "~r~Score:~w~ 1200";

    samp::net::BitStream stream;
    samp::protocol::WriteShowTextDraw(stream, sent);
    Check(stream.GetNumberOfBitsUsed() ==
              static_cast<int>(2 + samp::protocol::kTextDrawStyleSize + 2 + sent.text.size()) * 8,
          "a textdraw is its id, style block, length and text");

    samp::protocol::ShowTextDraw received;
    Check(samp::protocol::ReadShowTextDraw(stream, received), "textdraw reads back");
    Check(received.textDrawId == 12, "textdraw id round-trips");
    Check(received.style.flags == 0x0D, "style flags round-trip");
    Check(received.style.letterHeight == 1.5f, "letter size round-trips");
    Check(received.style.letterColour == 0xFFFFFFFF, "letter colour round-trips");
    Check(received.style.boxColour == 0x80000000, "box colour round-trips");
    Check(received.style.shadow == 1 && received.style.outline == 2,
          "shadow and outline round-trip");
    Check(received.style.x == 320.0f && received.style.y == 240.0f, "placement round-trips");
    Check(!received.style.IsModelPreview(), "an ordinary textdraw is not a model preview");
    Check(received.text == sent.text, "textdraw text round-trips");
}

void TestTextDrawModelPreview() {

    samp::protocol::ShowTextDraw sent;
    sent.textDrawId = 5;
    sent.style.style = samp::protocol::kTextDrawPreviewStyle;
    sent.style.previewModel = 411;
    sent.style.previewRotation = {0.0f, 0.0f, 45.0f};
    sent.style.previewZoom = 1.25f;
    sent.style.previewColour1 = 3;
    sent.style.previewColour2 = 7;
    sent.text = "preview";

    samp::net::BitStream stream;
    samp::protocol::WriteShowTextDraw(stream, sent);

    samp::protocol::ShowTextDraw received;
    Check(samp::protocol::ReadShowTextDraw(stream, received), "model preview reads back");
    Check(received.style.IsModelPreview(), "the preview style is recognised");
    Check(received.style.previewModel == 411, "preview model round-trips");
    Check(received.style.previewRotation.z == 45.0f, "preview rotation round-trips");
    Check(received.style.previewZoom == 1.25f, "preview zoom round-trips");
    Check(received.style.previewColour1 == 3 && received.style.previewColour2 == 7,
          "preview colours round-trip");
}

void TestTextDrawHideAndUpdate() {
    samp::net::BitStream hideStream;
    samp::protocol::WriteHideTextDraw(hideStream, {99});
    samp::protocol::HideTextDraw hidden;
    Check(samp::protocol::ReadHideTextDraw(hideStream, hidden) && hidden.textDrawId == 99,
          "hiding a textdraw round-trips");

    samp::net::BitStream updateStream;
    samp::protocol::WriteSetTextDrawString(updateStream, {99, "new text"});
    samp::protocol::SetTextDrawString updated;
    Check(samp::protocol::ReadSetTextDrawString(updateStream, updated) &&
              updated.text == "new text",
          "updating a textdraw string round-trips");
}

void TestTextDrawLimitsDifferByOne() {

    const std::string atLimit(samp::protocol::kMaxTextDrawTextLength, 'x');

    samp::net::BitStream updateStream;
    updateStream.Write<std::uint16_t>(1);
    updateStream.Write<std::uint16_t>(samp::protocol::kMaxTextDrawTextLength);
    updateStream.WriteBytes(atLimit.data(), atLimit.size());

    samp::protocol::SetTextDrawString updated;
    Check(samp::protocol::ReadSetTextDrawString(updateStream, updated),
          "the update message accepts text at the limit");

    samp::net::BitStream showStream;
    showStream.Write<std::uint16_t>(1);
    samp::protocol::WriteTextDrawStyle(showStream, samp::protocol::TextDrawStyle{});
    showStream.Write<std::uint16_t>(samp::protocol::kMaxTextDrawTextLength);
    showStream.WriteBytes(atLimit.data(), atLimit.size());

    samp::protocol::ShowTextDraw shown;
    Check(!samp::protocol::ReadShowTextDraw(showStream, shown),
          "the show message stops one byte earlier");
}

void TestPlayerNameRoundTrip() {
    samp::net::BitStream stream;
    samp::protocol::WriteSetPlayerName(stream, {4, "Nickname", 1});

    samp::protocol::SetPlayerName name;
    Check(samp::protocol::ReadSetPlayerName(stream, name), "player name reads back");
    Check(name.playerId == 4 && name.name == "Nickname", "name round-trips");
    Check(name.Accepted(), "a successful rename is reported as accepted");

    samp::net::BitStream rejectedStream;
    samp::protocol::WriteSetPlayerName(rejectedStream, {4, "Taken", 0});
    samp::protocol::SetPlayerName rejected;
    Check(samp::protocol::ReadSetPlayerName(rejectedStream, rejected), "rejected rename reads");
    Check(!rejected.Accepted(), "a rejected rename is not applied");
    Check(rejected.name == "Taken", "the attempted name is still readable");
}

void TestPlayerNameLengthIsCapped() {
    samp::net::BitStream stream;
    stream.Write<std::uint16_t>(1);
    stream.Write<std::uint8_t>(static_cast<std::uint8_t>(samp::protocol::kMaxPlayerNameLength + 1));

    samp::protocol::SetPlayerName name;
    Check(!samp::protocol::ReadSetPlayerName(stream, name),
          "a name longer than the protocol allows is rejected");
}

void TestGameTextRoundTrip() {
    samp::net::BitStream stream;
    samp::protocol::WriteDisplayGameText(stream, {4, 5000, "~g~GO!"});

    samp::protocol::DisplayGameText text;
    Check(samp::protocol::ReadDisplayGameText(stream, text), "game text reads back");
    Check(text.style == 4 && text.durationMs == 5000, "style and duration round-trip");
    Check(text.text == "~g~GO!", "banner text round-trips");
}

void TestEmptyGameTextIsRefused() {

    samp::net::BitStream stream;
    stream.Write<std::int32_t>(1);
    stream.Write<std::int32_t>(1000);
    stream.Write<std::int32_t>(0);

    samp::protocol::DisplayGameText text;
    Check(!samp::protocol::ReadDisplayGameText(stream, text), "an empty banner is refused");
}

void TestPlayerTeamRoundTrip() {
    samp::net::BitStream stream;
    samp::protocol::WriteSetPlayerTeam(stream, {200, 3});

    samp::protocol::SetPlayerTeam team;
    Check(samp::protocol::ReadSetPlayerTeam(stream, team) && team.playerId == 200 &&
              team.team == 3,
          "player team round-trips");
}

void TestSpawnInfoRoundTrip() {
    samp::protocol::SpawnInfo sent;
    sent.team = 2;
    sent.skin = 287;
    sent.position = {1958.33f, 1343.12f, 15.36f};
    sent.rotation = 269.15f;
    sent.weapons = {24, 31, samp::protocol::kNoWeapon};
    sent.ammo = {150, 500, 0};

    samp::net::BitStream stream;
    samp::protocol::WriteSpawnInfo(stream, sent);
    Check(stream.GetNumberOfBitsUsed() == static_cast<int>(samp::protocol::kSpawnInfoSize) * 8,
          "spawn info is forty-six bytes");

    samp::protocol::SpawnInfo received;
    Check(samp::protocol::ReadSpawnInfo(stream, received), "spawn info reads back");
    Check(received.team == 2 && received.skin == 287, "team and skin round-trip");
    Check(received.position.x == 1958.33f && received.rotation == 269.15f,
          "spawn placement round-trips");
    Check(received.weapons[0] == 24 && received.weapons[1] == 31, "weapons round-trip");
    Check(received.weapons[2] == samp::protocol::kNoWeapon, "an empty weapon slot survives");
    Check(received.ammo[0] == 150 && received.ammo[1] == 500, "ammunition round-trips");
}

void TestSpawnWeaponsAreNotInterleaved() {

    samp::protocol::SpawnInfo sent;
    sent.weapons = {1, 2, 3};
    sent.ammo = {10, 20, 30};

    samp::net::BitStream stream;
    samp::protocol::WriteSpawnInfo(stream, sent);

    samp::protocol::SpawnInfo received;
    Check(samp::protocol::ReadSpawnInfo(stream, received), "spawn info reads back");
    Check(received.weapons[1] == 2, "the second weapon is a weapon, not ammunition");
    Check(received.ammo[0] == 10, "ammunition starts after all three weapons");
}

void TestWorldBoundsOrder() {

    samp::net::BitStream stream;
    samp::protocol::WriteSetPlayerWorldBounds(stream, {1000.0f, -1000.0f, 500.0f, -500.0f});
    Check(stream.GetNumberOfBitsUsed() == 16 * 8, "world bounds are sixteen bytes");

    samp::protocol::SetPlayerWorldBounds bounds;
    Check(samp::protocol::ReadSetPlayerWorldBounds(stream, bounds), "world bounds read back");
    Check(bounds.maxX == 1000.0f && bounds.minX == -1000.0f, "the x pair keeps its order");
    Check(bounds.maxY == 500.0f && bounds.minY == -500.0f, "the y pair keeps its order");
    Check(bounds.maxX > bounds.minX && bounds.maxY > bounds.minY,
          "upper bounds precede lower ones on the wire");
}

void TestMapIconRoundTrip() {
    const auto green = static_cast<std::int32_t>(0xFF00FF00);

    samp::protocol::SetPlayerMapIcon sent;
    sent.iconId = 3;
    sent.position = {1500.0f, -900.0f, 12.0f};
    sent.markerType = 55;
    sent.colour = green;
    sent.style = 1;

    samp::net::BitStream stream;
    samp::protocol::WriteSetPlayerMapIcon(stream, sent);
    Check(stream.GetNumberOfBitsUsed() == 19 * 8, "a map icon is nineteen bytes");

    samp::protocol::SetPlayerMapIcon icon;
    Check(samp::protocol::ReadSetPlayerMapIcon(stream, icon), "map icon reads back");
    Check(icon.iconId == 3 && icon.markerType == 55, "icon slot and marker type round-trip");
    Check(icon.position.y == -900.0f, "icon position round-trips");
    Check(icon.colour == green, "icon colour round-trips");
    Check(icon.style == 1, "icon style round-trips");
}

void TestNumberPlateRoundTrip() {
    samp::net::BitStream stream;
    samp::protocol::WriteSetVehicleNumberPlate(stream, {404, "XYZ 123"});

    samp::protocol::SetVehicleNumberPlate plate;
    Check(samp::protocol::ReadSetVehicleNumberPlate(stream, plate), "number plate reads back");
    Check(plate.vehicleId == 404 && plate.plate == "XYZ 123", "number plate round-trips");

    samp::net::BitStream oversized;
    oversized.Write<std::uint16_t>(1);
    oversized.Write<std::uint8_t>(
        static_cast<std::uint8_t>(samp::protocol::kMaxNumberPlateLength + 1));
    Check(!samp::protocol::ReadSetVehicleNumberPlate(oversized, plate),
          "a plate longer than the client accepts is rejected");
}

void TestTrailerMessages() {

    samp::net::BitStream attachStream;
    samp::protocol::WriteAttachTrailerToVehicle(attachStream, {17, 42});

    samp::protocol::AttachTrailerToVehicle attached;
    Check(samp::protocol::ReadAttachTrailerToVehicle(attachStream, attached),
          "trailer attachment reads back");
    Check(attached.trailerId == 17, "the first id on the wire is the trailer");
    Check(attached.vehicleId == 42, "the second id is the towing vehicle");

    samp::net::BitStream detachStream;
    samp::protocol::WriteDetachTrailerFromVehicle(detachStream, {42});
    Check(detachStream.GetNumberOfBitsUsed() == 2 * 8, "unhitching names one vehicle only");

    samp::protocol::DetachTrailerFromVehicle detached;
    Check(samp::protocol::ReadDetachTrailerFromVehicle(detachStream, detached) &&
              detached.vehicleId == 42,
          "trailer detachment round-trips");
}

void TestVehicleComponentAndInterior() {
    samp::net::BitStream componentStream;
    samp::protocol::WriteRemoveVehicleComponent(componentStream, {88, 1010});

    samp::protocol::RemoveVehicleComponent component;
    Check(samp::protocol::ReadRemoveVehicleComponent(componentStream, component) &&
              component.componentId == 1010,
          "component removal round-trips");

    samp::net::BitStream interiorStream;
    samp::protocol::WriteLinkVehicleToInterior(interiorStream, {88, 5});

    samp::protocol::LinkVehicleToInterior link;
    Check(samp::protocol::ReadLinkVehicleToInterior(interiorStream, link) && link.interiorId == 5,
          "interior link round-trips");
}

void TestRemoveBuildingRoundTrip() {
    samp::net::BitStream stream;
    samp::protocol::WriteRemoveBuildingForPlayer(stream, {1234, {100.0f, 200.0f, 30.0f}, 50.0f});
    Check(stream.GetNumberOfBitsUsed() == 20 * 8, "building removal is twenty bytes");

    samp::protocol::RemoveBuildingForPlayer removal;
    Check(samp::protocol::ReadRemoveBuildingForPlayer(stream, removal), "removal reads back");
    Check(removal.modelId == 1234 && removal.radius == 50.0f, "model and radius round-trip");
    Check(removal.position.z == 30.0f, "removal centre round-trips");
}

void TestDeathMessageWithAndWithoutKiller() {
    samp::net::BitStream killStream;
    samp::protocol::WriteDeathMessage(killStream, {5, 9, 24});
    Check(killStream.GetNumberOfBitsUsed() == 5 * 8, "a death message is five bytes");

    samp::protocol::DeathMessage kill;
    Check(samp::protocol::ReadDeathMessage(killStream, kill), "death message reads back");
    Check(kill.HasKiller() && kill.killerId == 5, "the killer is reported");
    Check(kill.victimId == 9 && kill.reason == 24, "victim and reason round-trip");
    Check(kill.IsValid(), "a message naming a victim is usable");

    samp::net::BitStream soloStream;
    samp::protocol::WriteDeathMessage(soloStream, {samp::protocol::kInvalidPlayerId, 9, 54});

    samp::protocol::DeathMessage solo;
    Check(samp::protocol::ReadDeathMessage(soloStream, solo), "killerless death reads back");
    Check(!solo.HasKiller(), "an absent killer is recognised rather than read as player 65535");
    Check(solo.IsValid(), "a killerless death is still a valid message");
}

void TestSelectTextDrawIsBitPacked() {
    samp::net::BitStream stream;
    samp::protocol::WriteSelectTextDraw(stream, {true, 0xFF0000FF});

    Check(stream.GetNumberOfBitsUsed() == 33, "textdraw selection is thirty-three bits");

    samp::protocol::SelectTextDraw selection;
    Check(samp::protocol::ReadSelectTextDraw(stream, selection), "selection reads back");
    Check(selection.enabled, "the selection flag round-trips");
    Check(selection.hoverColour == 0xFF0000FF, "the hover colour survives the bit offset");

    samp::net::BitStream offStream;
    samp::protocol::WriteSelectTextDraw(offStream, {false, 0});
    samp::protocol::SelectTextDraw cancelled;
    Check(samp::protocol::ReadSelectTextDraw(offStream, cancelled) && !cancelled.enabled,
          "cancelling selection round-trips");
}

void Test3DTextLabelRoundTrip() {
    samp::protocol::Create3DTextLabel sent;
    sent.labelId = 700;
    sent.colour = 0xFF00FFFF;
    sent.position = {10.0f, 20.0f, 3.5f};
    sent.drawDistance = 40.0f;
    sent.testLineOfSight = 1;
    sent.attachedPlayerId = 12;
    sent.text = "Shop entrance";

    samp::net::BitStream stream;
    samp::protocol::WriteCreate3DTextLabel(stream, sent);

    samp::protocol::Create3DTextLabel received;
    Check(samp::protocol::ReadCreate3DTextLabel(stream, received), "3D label reads back");
    Check(received.labelId == 700 && received.colour == 0xFF00FFFF, "label identity round-trips");
    Check(received.position.z == 3.5f && received.drawDistance == 40.0f,
          "label placement round-trips");
    Check(received.testLineOfSight == 1, "line-of-sight flag round-trips");
    Check(received.attachedPlayerId == 12, "player attachment round-trips");
    Check(received.text == "Shop entrance", "compressed label text round-trips");
    Check(stream.GetNumberOfUnreadBits() == 0, "the message is consumed exactly");

    samp::net::BitStream removeStream;
    samp::protocol::WriteRemove3DTextLabel(removeStream, {700});
    samp::protocol::Remove3DTextLabel removed;
    Check(samp::protocol::ReadRemove3DTextLabel(removeStream, removed) && removed.labelId == 700,
          "label removal round-trips");
}

void TestCheckpointRoundTrip() {
    samp::net::BitStream stream;
    samp::protocol::WriteSetCheckpoint(stream, {{100.0f, 200.0f, 10.0f}, 5.5f});
    Check(stream.GetNumberOfBitsUsed() == 16 * 8, "a checkpoint is sixteen bytes");

    samp::protocol::SetCheckpoint checkpoint;
    Check(samp::protocol::ReadSetCheckpoint(stream, checkpoint), "checkpoint reads back");
    Check(checkpoint.position.y == 200.0f, "checkpoint position round-trips");
    Check(checkpoint.size == 5.5f, "the single size value round-trips");
}

void TestSkillLevelRoundTrip() {
    samp::net::BitStream stream;
    samp::protocol::WriteSetPlayerSkillLevel(stream, {3, 4, 999});
    Check(stream.GetNumberOfBitsUsed() == 8 * 8, "a skill update is eight bytes");

    samp::protocol::SetPlayerSkillLevel skill;
    Check(samp::protocol::ReadSetPlayerSkillLevel(stream, skill), "skill level reads back");
    Check(skill.playerId == 3 && skill.skillType == 4 && skill.level == 999,
          "skill fields round-trip");
}

void TestRaceCheckpointRoundTrip() {
    samp::net::BitStream stream;
    samp::protocol::WriteSetRaceCheckpoint(
        stream, {2, {10.0f, 20.0f, 30.0f}, {40.0f, 50.0f, 60.0f}, 8.0f});
    Check(stream.GetNumberOfBitsUsed() == 29 * 8, "a race checkpoint is twenty-nine bytes");

    samp::protocol::SetRaceCheckpoint checkpoint;
    Check(samp::protocol::ReadSetRaceCheckpoint(stream, checkpoint), "race checkpoint reads back");
    Check(checkpoint.type == 2, "checkpoint type round-trips");
    Check(checkpoint.position.x == 10.0f, "checkpoint position round-trips");
    Check(checkpoint.nextPosition.x == 40.0f, "the next checkpoint position round-trips");
    Check(checkpoint.size == 8.0f, "checkpoint size round-trips");
}

void TestChatBubbleRoundTrip() {
    samp::net::BitStream stream;
    samp::protocol::WriteChatBubble(stream, {7, 0xFFFFFFFF, 25.0f, 3000, "over here"});

    samp::protocol::ChatBubble bubble;
    Check(samp::protocol::ReadChatBubble(stream, bubble), "chat bubble reads back");
    Check(bubble.playerId == 7 && bubble.colour == 0xFFFFFFFF, "bubble owner and colour survive");
    Check(bubble.drawDistance == 25.0f, "bubble draw distance round-trips");
    Check(bubble.expireTimeMs == 3000, "bubble lifetime round-trips");
    Check(bubble.text == "over here", "bubble text round-trips");

    samp::net::BitStream oversized;
    oversized.Write<std::uint16_t>(1);
    oversized.Write<std::uint32_t>(0);
    oversized.Write<float>(0.0f);
    oversized.Write<std::int32_t>(0);
    oversized.Write<std::uint8_t>(
        static_cast<std::uint8_t>(samp::protocol::kMaxChatBubbleLength + 1));
    Check(!samp::protocol::ReadChatBubble(oversized, bubble),
          "a bubble longer than the client accepts is rejected");
}

void TestWorldPlayerEvents() {
    samp::net::BitStream deathStream;
    samp::protocol::WriteWorldPlayerDeath(deathStream, {33});
    Check(deathStream.GetNumberOfBitsUsed() == 2 * 8, "a death event is a single id");

    samp::protocol::WorldPlayerDeath death;
    Check(samp::protocol::ReadWorldPlayerDeath(deathStream, death) && death.playerId == 33,
          "death event round-trips");

    samp::net::BitStream removeStream;
    samp::protocol::WriteWorldPlayerRemove(removeStream, {33});
    samp::protocol::WorldPlayerRemove removal;
    Check(samp::protocol::ReadWorldPlayerRemove(removeStream, removal) && removal.playerId == 33,
          "removal event round-trips");
}

void TestWorldPlayerAddRoundTrip() {
    samp::protocol::WorldPlayerAdd sent;
    sent.playerId = 40;
    sent.team = 3;
    sent.skin = 217;
    sent.position = {2000.5f, -1500.25f, 20.0f};
    sent.rotation = 90.0f;
    sent.colour = 0x1122'3344;
    sent.fightingStyle = 5;
    for (std::size_t i = 0; i < sent.skillLevels.size(); ++i) {
        sent.skillLevels[i] = static_cast<std::uint16_t>(100 * (i + 1));
    }

    samp::net::BitStream stream;
    samp::protocol::WriteWorldPlayerAdd(stream, sent);
    Check(stream.GetNumberOfBitsUsed() == 50 * 8, "streaming a player in is fifty bytes");

    samp::protocol::WorldPlayerAdd received;
    Check(samp::protocol::ReadWorldPlayerAdd(stream, received), "player add reads back");
    Check(received.playerId == 40 && received.skin == 217, "identity round-trips");
    Check(received.HasTeam() && received.team == 3, "team round-trips");
    Check(received.position.y == -1500.25f && received.rotation == 90.0f,
          "spawn placement round-trips");
    Check(received.fightingStyle == 5, "fighting style round-trips");
    Check(received.skillLevels == sent.skillLevels, "all eleven skill levels round-trip");
}

void TestTeamlessPlayerIsRecognised() {

    samp::protocol::WorldPlayerAdd sent;
    sent.playerId = 1;
    sent.team = samp::protocol::kNoTeam;

    samp::net::BitStream stream;
    samp::protocol::WriteWorldPlayerAdd(stream, sent);

    samp::protocol::WorldPlayerAdd received;
    Check(samp::protocol::ReadWorldPlayerAdd(stream, received), "teamless player reads back");
    Check(!received.HasTeam(), "the no-team marker is recognised");
}

void TestWorldVehicleAddRoundTrip() {
    samp::protocol::WorldVehicleAdd sent;
    sent.vehicleId = 250;
    sent.modelId = 411;
    sent.position = {1000.0f, 2000.0f, 12.5f};
    sent.rotation = 180.0f;
    sent.colour1 = 3;
    sent.colour2 = 6;
    sent.health = 950.0f;
    sent.interiorColour = 2;
    sent.doorStatus = 0x0000FFFF;
    sent.panelStatus = 0x00FF00FF;
    sent.lightStatus = 0x03;
    sent.tyreStatus = 0x05;
    sent.addSiren = 1;
    sent.modSlots[0] = 25;
    sent.modSlots[13] = 99;
    sent.paintjob = 3;
    sent.modColour1 = 12;
    sent.modColour2 = 34;

    samp::net::BitStream stream;
    samp::protocol::WriteWorldVehicleAdd(stream, sent);
    Check(stream.GetNumberOfBitsUsed() == 63 * 8, "streaming a vehicle in is sixty-three bytes");

    samp::protocol::WorldVehicleAdd received;
    Check(samp::protocol::ReadWorldVehicleAdd(stream, received), "vehicle add reads back");
    Check(received.vehicleId == 250 && received.modelId == 411, "identity round-trips");
    Check(received.HasModelInRange(), "the model sits inside the accepted range");
    Check(received.position.z == 12.5f && received.rotation == 180.0f, "placement round-trips");
    Check(received.health == 950.0f, "health round-trips");
    Check(received.doorStatus == 0x0000FFFF && received.panelStatus == 0x00FF00FF,
          "damage words keep their order");
    Check(received.tyreStatus == 0x05 && received.lightStatus == 0x03,
          "the damage bytes are not swapped");
    Check(received.modSlots == sent.modSlots, "all fourteen modification slots round-trip");
    Check(received.HasPaintjob() && received.paintjob == 3, "paint job round-trips");
    Check(received.modColour1 == 12 && received.modColour2 == 34,
          "modification colours round-trip");
}

void TestVehicleMarkersAreRecognised() {
    samp::protocol::WorldVehicleAdd sent;
    sent.modelId = samp::protocol::kMaxVehicleModel + 1;
    Check(!sent.HasModelInRange(), "a model past the range is rejected by the check");

    sent.modelId = samp::protocol::kMinVehicleModel;
    Check(sent.HasModelInRange(), "the first valid model is accepted");

    const samp::protocol::WorldVehicleAdd fresh;
    Check(!fresh.HasOwnColours(), "the default colours mean no override");
    Check(!fresh.HasPaintjob(), "a zero paint job means none");
    Check(fresh.modColour1 == samp::protocol::kNoModColour,
          "an unset modification colour is not colour zero");
}

void TestServerJoinAndQuit() {
    samp::net::BitStream joinStream;
    samp::protocol::WriteServerJoin(joinStream, {11, 0x00FF0000, 0, "Newcomer"});

    samp::protocol::ServerJoin join;
    Check(samp::protocol::ReadServerJoin(joinStream, join), "join reads back");
    Check(join.playerId == 11 && join.name == "Newcomer", "joining player round-trips");
    Check(join.HasColour(), "a non-zero colour is recognised");
    Check(join.isNpc == 0, "the bot flag round-trips");

    samp::net::BitStream defaultColour;
    samp::protocol::WriteServerJoin(defaultColour, {12, 0, 1, "Bot"});
    samp::protocol::ServerJoin bot;
    Check(samp::protocol::ReadServerJoin(defaultColour, bot), "bot join reads back");
    Check(!bot.HasColour(), "a zero colour means the default, not black");
    Check(bot.isNpc == 1, "the bot flag survives");

    samp::net::BitStream quitStream;
    samp::protocol::WriteServerQuit(quitStream, {11, 2});
    Check(quitStream.GetNumberOfBitsUsed() == 3 * 8, "a quit message is three bytes");

    samp::protocol::ServerQuit quit;
    Check(samp::protocol::ReadServerQuit(quitStream, quit) && quit.reason == 2,
          "quit reason round-trips");
}

void TestConnectionRejected() {
    samp::net::BitStream stream;
    samp::protocol::WriteConnectionRejected(
        stream, {static_cast<std::uint8_t>(samp::protocol::RejectReason::BadModVersion)});

    samp::protocol::ConnectionRejected rejected;
    Check(samp::protocol::ReadConnectionRejected(stream, rejected), "rejection reads back");
    Check(rejected.reason == static_cast<std::uint8_t>(samp::protocol::RejectReason::BadModVersion),
          "the rejection reason round-trips");
}

void TestScoreUpdateIsLengthDriven() {

    const std::vector<samp::protocol::ScoreEntry> sent = {{1, 100, 45}, {2, -50, 120}, {3, 0, 7}};

    samp::net::BitStream stream;
    samp::protocol::WriteScoreUpdate(stream, sent);
    Check(stream.GetNumberOfBitsUsed() ==
              static_cast<int>(sent.size() * samp::protocol::kScoreEntrySize) * 8,
          "each scoreboard entry is ten bytes");

    std::vector<samp::protocol::ScoreEntry> received;
    Check(samp::protocol::ReadScoreUpdate(stream, received), "the scoreboard reads back");
    Check(received.size() == 3, "the entry count comes from the packet length");
    Check(received[1].playerId == 2 && received[1].score == -50 && received[1].ping == 120,
          "a negative score round-trips");
    Check(stream.GetNumberOfUnreadBits() == 0, "the packet is consumed exactly");
}

void TestScoreUpdateIgnoresTrailingScrap() {

    samp::net::BitStream stream;
    samp::protocol::WriteScoreUpdate(stream, {{1, 10, 20}});
    stream.Write<std::uint16_t>(9);

    std::vector<samp::protocol::ScoreEntry> received;
    Check(samp::protocol::ReadScoreUpdate(stream, received), "the whole entries still read");
    Check(received.size() == 1, "a partial trailing entry is left alone");
}

void TestInitGameRoundTrip() {
    samp::protocol::InitGame sent;
    sent.flag1 = true;
    sent.flag4 = true;
    sent.value1 = 0x11223344;
    sent.stuntBonusEnabled = true;
    sent.value2 = 7;
    sent.playerId = 42;
    sent.weather = 11;
    sent.gravity = 0x3B23D70A;
    sent.deathMode = 2;
    sent.hostname = "Test Server 01";
    sent.modelBlock[0] = 0xAA;
    sent.modelBlock[211] = 0xBB;
    sent.value10 = 1;

    samp::net::BitStream stream;
    samp::protocol::WriteInitGame(stream, sent);

    Check(stream.GetNumberOfBitsUsed() % 8 != 0, "the init packet is not byte-aligned");

    samp::protocol::InitGame received;
    Check(samp::protocol::ReadInitGame(stream, received), "the init packet reads back");
    Check(received.playerId == 42, "the assigned player id round-trips");
    Check(received.hostname == "Test Server 01", "the server name round-trips");
    Check(received.weather == 11 && received.gravity == 0x3B23D70A,
          "weather and gravity round-trip");
    Check(received.stuntBonusEnabled && received.flag1 && received.flag4,
          "the flags that were set survive");
    Check(!received.flag2 && !received.flag3, "the flags that were clear stay clear");
    Check(received.deathMode == 2, "the death mode round-trips");
    Check(received.modelBlock == sent.modelBlock, "the model block survives untouched");
    Check(received.value10 == 1, "the trailing value round-trips");
    Check(stream.GetNumberOfUnreadBits() == 0, "the packet is consumed exactly");
}

void TestInitGameFlagsAreSingleBits() {

    samp::protocol::InitGame withoutFlag;
    samp::protocol::InitGame withFlag;
    withFlag.flag9 = true;

    samp::net::BitStream first;
    samp::net::BitStream second;
    samp::protocol::WriteInitGame(first, withoutFlag);
    samp::protocol::WriteInitGame(second, withFlag);

    Check(first.GetNumberOfBitsUsed() == second.GetNumberOfBitsUsed(),
          "a flag occupies the same single bit whether set or clear");
}

void TestActorLifecycle() {
    samp::protocol::ShowActor sent;
    sent.actorId = 15;
    sent.modelId = 287;
    sent.position = {500.0f, -600.0f, 10.0f};
    sent.rotation = 45.0f;
    sent.health = 100.0f;
    sent.visible = 1;

    samp::net::BitStream stream;
    samp::protocol::WriteShowActor(stream, sent);
    Check(stream.GetNumberOfBitsUsed() == 27 * 8, "showing an actor is twenty-seven bytes");

    samp::protocol::ShowActor received;
    Check(samp::protocol::ReadShowActor(stream, received), "actor reads back");
    Check(received.actorId == 15 && received.modelId == 287, "actor identity round-trips");
    Check(received.position.y == -600.0f && received.rotation == 45.0f,
          "actor placement round-trips");
    Check(received.health == 100.0f && received.visible == 1, "health and visibility round-trip");

    samp::net::BitStream hideStream;
    samp::protocol::WriteHideActor(hideStream, {15});
    samp::protocol::HideActor hidden;
    Check(samp::protocol::ReadHideActor(hideStream, hidden) && hidden.actorId == 15,
          "hiding an actor round-trips");
}

void TestActorUpdates() {
    samp::net::BitStream positionStream;
    samp::protocol::WriteSetActorPosition(positionStream, {15, {1.0f, 2.0f, 3.0f}});
    Check(positionStream.GetNumberOfBitsUsed() == 14 * 8, "an actor move is fourteen bytes");

    samp::protocol::SetActorPosition position;
    Check(samp::protocol::ReadSetActorPosition(positionStream, position) &&
              position.position.z == 3.0f,
          "actor position round-trips");

    samp::net::BitStream healthStream;
    samp::protocol::WriteSetActorHealth(healthStream, {15, 55.5f});
    samp::protocol::SetActorHealth health;
    Check(samp::protocol::ReadSetActorHealth(healthStream, health) && health.health == 55.5f,
          "actor health round-trips");

    samp::net::BitStream angleStream;
    samp::protocol::WriteSetActorFacingAngle(angleStream, {15, 270.0f});
    samp::protocol::SetActorFacingAngle angle;
    Check(samp::protocol::ReadSetActorFacingAngle(angleStream, angle) && angle.angle == 270.0f,
          "actor facing angle round-trips");
}

void TestActorAnimationRoundTrip() {
    samp::protocol::ApplyActorAnimation sent;
    sent.actorId = 4;
    sent.animationLibrary = "PED";
    sent.animationName = "IDLE_CHAT";
    sent.delta = 4.1f;
    sent.loop = true;
    sent.lockY = true;
    sent.timeMs = 0;

    samp::net::BitStream stream;
    samp::protocol::WriteApplyActorAnimation(stream, sent);

    const int expected = (2 + 1 + 3 + 1 + 9 + 4) * 8 + 4 + 32;
    Check(stream.GetNumberOfBitsUsed() == expected, "the four switches cost one bit each");

    samp::protocol::ApplyActorAnimation received;
    Check(samp::protocol::ReadApplyActorAnimation(stream, received), "actor animation reads back");
    Check(received.actorId == 4, "actor id round-trips");
    Check(received.animationLibrary == "PED", "animation library round-trips");
    Check(received.animationName == "IDLE_CHAT", "animation name round-trips");
    Check(received.delta == 4.1f, "animation delta round-trips");
    Check(received.loop && received.lockY, "the switches that were set survive");
    Check(!received.lockX && !received.freeze, "the switches that were clear stay clear");
    Check(stream.GetNumberOfUnreadBits() == 0, "the message is consumed exactly");
}

void TestAnimationClearing() {
    samp::net::BitStream actorStream;
    samp::protocol::WriteClearActorAnimation(actorStream, {4});
    samp::protocol::ClearActorAnimation clearedActor;
    Check(samp::protocol::ReadClearActorAnimation(actorStream, clearedActor) &&
              clearedActor.actorId == 4,
          "clearing an actor animation round-trips");

    samp::net::BitStream playerStream;
    samp::protocol::WriteClearAnimations(playerStream, {9});
    samp::protocol::ClearAnimations clearedPlayer;
    Check(samp::protocol::ReadClearAnimations(playerStream, clearedPlayer) &&
              clearedPlayer.playerId == 9,
          "clearing a player animation round-trips");
}

void TestAttachedObjectRoundTrip() {
    samp::protocol::SetPlayerAttachedObject sent;
    sent.playerId = 6;
    sent.slot = 2;
    sent.attached = true;
    sent.object.modelId = 19086;
    sent.object.boneId = 2;
    sent.object.offset = {0.1f, 0.2f, 0.3f};
    sent.object.rotation = {0.0f, 90.0f, 0.0f};
    sent.object.scale = {1.0f, 1.0f, 1.0f};
    sent.object.materialColour1 = 0xFF112233;
    sent.object.materialColour2 = 0xFF445566;

    samp::net::BitStream stream;
    samp::protocol::WriteSetPlayerAttachedObject(stream, sent);

    Check(stream.GetNumberOfBitsUsed() == (2 + 4 + 52) * 8 + 1,
          "an attachment carries the full description");

    samp::protocol::SetPlayerAttachedObject received;
    Check(samp::protocol::ReadSetPlayerAttachedObject(stream, received), "attachment reads back");
    Check(received.attached && received.slot == 2, "slot and flag round-trip");
    Check(received.HasValidSlot(), "the slot is inside the range");
    Check(received.object.modelId == 19086 && received.object.HasValidBone(),
          "model and bone round-trip");
    Check(received.object.rotation.y == 90.0f, "attachment rotation round-trips");
    Check(received.object.materialColour2 == 0xFF445566, "material colours round-trip");
}

void TestClearingAttachmentIsShorter() {

    samp::protocol::SetPlayerAttachedObject sent;
    sent.playerId = 6;
    sent.slot = 2;
    sent.attached = false;

    samp::net::BitStream stream;
    samp::protocol::WriteSetPlayerAttachedObject(stream, sent);
    Check(stream.GetNumberOfBitsUsed() == 6 * 8 + 1, "clearing a slot is six bytes and a bit");

    samp::protocol::SetPlayerAttachedObject received;
    Check(samp::protocol::ReadSetPlayerAttachedObject(stream, received), "clearing reads back");
    Check(!received.attached, "the clear flag round-trips");
    Check(stream.GetNumberOfUnreadBits() == 0, "nothing is left unread");
}

void TestSmallMessages() {
    samp::net::BitStream toggleStream;
    samp::protocol::WriteToggleCameraTarget(toggleStream, {true});
    Check(toggleStream.GetNumberOfBitsUsed() == 1, "a camera-target toggle is a single bit");

    samp::protocol::ToggleCameraTarget toggle;
    Check(samp::protocol::ReadToggleCameraTarget(toggleStream, toggle) && toggle.enabled,
          "the camera-target toggle round-trips");

    samp::net::BitStream eventStream;
    samp::protocol::WriteScmEvent(eventStream, {1, 2, 3, 4, 5});
    Check(eventStream.GetNumberOfBitsUsed() == 18 * 8, "a script event is eighteen bytes");

    samp::protocol::ScmEvent event;
    Check(samp::protocol::ReadScmEvent(eventStream, event) && event.value4 == 5,
          "the script event round-trips");

    samp::protocol::ServerNetStats stats;
    stats.data[0] = 0xAB;
    stats.data[samp::protocol::kServerNetStatsSize - 1] = 0xCD;

    samp::net::BitStream statsStream;
    samp::protocol::WriteServerNetStats(statsStream, stats);

    samp::protocol::ServerNetStats readStats;
    Check(samp::protocol::ReadServerNetStats(statsStream, readStats) && readStats.data == stats.data,
          "the statistics blob survives untouched");
}

void TestShopNameIsFixedWidth() {
    samp::net::BitStream stream;
    samp::protocol::WriteSetPlayerShopName(stream, {"Ammunation"});

    Check(stream.GetNumberOfBitsUsed() == static_cast<int>(samp::protocol::kShopNameSize) * 8,
          "a shop name always occupies thirty-two bytes");

    samp::protocol::SetPlayerShopName shop;
    Check(samp::protocol::ReadSetPlayerShopName(stream, shop), "shop name reads back");
    Check(shop.name == "Ammunation", "the name stops at the terminator, not at the field end");
    Check(!shop.ClosesShop(), "a named shop does not read as a close instruction");

    samp::net::BitStream emptyStream;
    samp::protocol::WriteSetPlayerShopName(emptyStream, {""});
    Check(emptyStream.GetNumberOfBitsUsed() == static_cast<int>(samp::protocol::kShopNameSize) * 8,
          "an empty name still occupies the full field");

    samp::protocol::SetPlayerShopName closed;
    Check(samp::protocol::ReadSetPlayerShopName(emptyStream, closed), "empty name reads back");
    Check(closed.ClosesShop(), "an empty name means close the interface");
}

void TestAudioStreamAndDrunkLevel() {
    samp::net::BitStream audioStream;
    samp::protocol::WritePlayAudioStream(
        audioStream, {"http://example.invalid/radio", {10.0f, 20.0f, 30.0f}, 50.0f, 1});

    samp::protocol::PlayAudioStream audio;
    Check(samp::protocol::ReadPlayAudioStream(audioStream, audio), "audio stream reads back");
    Check(audio.url == "http://example.invalid/radio", "stream url round-trips");
    Check(audio.position.y == 20.0f && audio.distance == 50.0f,
          "stream anchor and range round-trip");
    Check(audio.usePosition == 1, "the positional flag round-trips");

    samp::net::BitStream drunkStream;
    samp::protocol::WriteSetPlayerDrunkLevel(drunkStream, {2000});
    samp::protocol::SetPlayerDrunkLevel drunk;
    Check(samp::protocol::ReadSetPlayerDrunkLevel(drunkStream, drunk) && drunk.level == 2000,
          "drunk level round-trips");
}

void TestEmptyBodiedMessage() {

    samp::net::BitStream stream;
    samp::protocol::CancelEdit sent;
    samp::protocol::WriteCancelEdit(stream, sent);
    Check(stream.GetNumberOfBitsUsed() == 0, "cancelling an edit writes nothing");

    samp::protocol::CancelEdit received;
    Check(samp::protocol::ReadCancelEdit(stream, received),
          "an empty body reads successfully rather than failing on no data");
}

void TestObjectEditingMessages() {
    samp::net::BitStream editStream;
    samp::protocol::WriteEditObject(editStream, {true, 700});
    Check(editStream.GetNumberOfBitsUsed() == 17, "editing an object is seventeen bits");

    samp::protocol::EditObject edit;
    Check(samp::protocol::ReadEditObject(editStream, edit), "edit request reads back");
    Check(edit.isPlayerObject && edit.objectId == 700, "edit target round-trips");

    samp::net::BitStream slotStream;
    samp::protocol::WriteEditAttachedObject(slotStream, {4});
    samp::protocol::EditAttachedObject slot;
    Check(samp::protocol::ReadEditAttachedObject(slotStream, slot) && slot.slot == 4,
          "editing an attachment slot round-trips");
}

void TestAttachObjectToPlayer() {
    samp::net::BitStream stream;
    samp::protocol::WriteAttachObjectToPlayer(
        stream, {700, 8, {0.5f, 0.0f, 1.0f}, {0.0f, 0.0f, 180.0f}});
    Check(stream.GetNumberOfBitsUsed() == 28 * 8, "attaching an object is twenty-eight bytes");

    samp::protocol::AttachObjectToPlayer attach;
    Check(samp::protocol::ReadAttachObjectToPlayer(stream, attach), "attachment reads back");
    Check(attach.objectId == 700 && attach.playerId == 8, "both ids round-trip");
    Check(attach.offset.x == 0.5f && attach.rotation.z == 180.0f,
          "offset and rotation keep their order");
}

void TestClientCheckRoundTrip() {
    samp::protocol::ClientCheckRequest sent;
    sent.type = static_cast<std::uint8_t>(samp::protocol::ClientCheckType::GameMemory);
    sent.target = 0x500000;
    sent.offset = 16;
    sent.length = 32;

    samp::net::BitStream stream;
    samp::protocol::WriteClientCheckRequest(stream, sent);
    Check(stream.GetNumberOfBitsUsed() == 9 * 8, "a check request is nine bytes");

    samp::protocol::ClientCheckRequest request;
    Check(samp::protocol::ReadClientCheckRequest(stream, request), "check request reads back");
    Check(request.target == 0x500000 && request.offset == 16 && request.length == 32,
          "the request fields round-trip");
    Check(request.IsWithinBounds(), "a well-formed request passes its bounds check");

    samp::net::BitStream replyStream;
    samp::protocol::WriteClientCheckResponse(replyStream, {sent.type, 0x500000, 1});
    Check(replyStream.GetNumberOfBitsUsed() == 6 * 8, "a check reply is six bytes");

    samp::protocol::ClientCheckResponse response;
    Check(samp::protocol::ReadClientCheckResponse(replyStream, response), "reply reads back");
    Check(response.result == 1 && response.value == 0x500000, "the reply fields round-trip");
}

void TestClientCheckBoundsAreChecked() {
    samp::protocol::ClientCheckRequest request;
    request.offset = samp::protocol::kMaxClientCheckOffset;
    request.length = samp::protocol::kMinClientCheckLength;
    Check(request.IsWithinBounds(), "the extreme values that are allowed pass");

    request.length = 1;
    Check(!request.IsWithinBounds(), "a length below the minimum is refused");

    request.length = samp::protocol::kMaxClientCheckLength + 1;
    Check(!request.IsWithinBounds(), "a length above the maximum is refused");

    request.length = samp::protocol::kMaxClientCheckLength;
    request.offset = samp::protocol::kMaxClientCheckOffset + 1;
    Check(!request.IsWithinBounds(), "an offset past the maximum is refused");
}

void TestObjectControlMessages() {
    samp::net::BitStream speedStream;
    samp::protocol::WriteSetObjectSpeed(speedStream, {700, {1.0f, 0.0f, -0.5f}});
    Check(speedStream.GetNumberOfBitsUsed() == 14 * 8, "an object speed is fourteen bytes");

    samp::protocol::SetObjectSpeed speed;
    Check(samp::protocol::ReadSetObjectSpeed(speedStream, speed) && speed.speed.z == -0.5f,
          "object speed round-trips");

    samp::net::BitStream collisionStream;
    samp::protocol::WriteSetObjectCollision(collisionStream, {700});
    Check(collisionStream.GetNumberOfBitsUsed() == 2 * 8,
          "enabling collision carries no value to switch");

    samp::protocol::SetObjectCollision collision;
    Check(samp::protocol::ReadSetObjectCollision(collisionStream, collision) &&
              collision.objectId == 700,
          "collision target round-trips");

    samp::net::BitStream rotationStream;
    samp::protocol::WriteSetObjectCurrentRotation(rotationStream, {700, 42});
    samp::protocol::SetObjectCurrentRotation rotation;
    Check(samp::protocol::ReadSetObjectCurrentRotation(rotationStream, rotation) &&
              rotation.value == 42,
          "object rotation round-trips");

    samp::net::BitStream clearStream;
    samp::protocol::WriteClearObjectMovement(clearStream, {700});
    samp::protocol::ClearObjectMovement cleared;
    Check(samp::protocol::ReadClearObjectMovement(clearStream, cleared) && cleared.objectId == 700,
          "clearing object movement round-trips");
}

void TestWidescreenToggle() {
    samp::net::BitStream stream;
    samp::protocol::WriteSetWidescreen(stream, {1});
    Check(stream.GetNumberOfBitsUsed() == 8, "the widescreen toggle is a whole byte, not a bit");

    samp::protocol::SetWidescreen widescreen;
    Check(samp::protocol::ReadSetWidescreen(stream, widescreen) && widescreen.enabled == 1,
          "the widescreen toggle round-trips");
}

void TestVehicleSeatingAndSkin() {
    samp::net::BitStream seatStream;
    samp::protocol::WritePutPlayerInVehicle(seatStream, {404, 2});
    Check(seatStream.GetNumberOfBitsUsed() == 3 * 8, "seating a player is three bytes");

    samp::protocol::PutPlayerInVehicle seat;
    Check(samp::protocol::ReadPutPlayerInVehicle(seatStream, seat) && seat.vehicleId == 404 &&
              seat.seatId == 2,
          "vehicle and seat round-trip");

    samp::net::BitStream skinStream;
    samp::protocol::WriteSetPlayerSkin(skinStream, {3, 287});
    Check(skinStream.GetNumberOfBitsUsed() == 8 * 8, "a skin change is eight bytes");

    samp::protocol::SetPlayerSkin skin;
    Check(samp::protocol::ReadSetPlayerSkin(skinStream, skin) && skin.skinId == 287,
          "skin round-trips");
}

void TestFindZAndVehicleParams() {
    samp::net::BitStream findZStream;
    samp::protocol::WriteSetPlayerPositionFindZ(findZStream, {{100.0f, 200.0f, 5.0f}});
    Check(findZStream.GetNumberOfBitsUsed() == 12 * 8, "a find-z teleport is twelve bytes");

    samp::protocol::SetPlayerPositionFindZ findZ;
    Check(samp::protocol::ReadSetPlayerPositionFindZ(findZStream, findZ) &&
              findZ.position.z == 5.0f,
          "the starting height round-trips");

    samp::net::BitStream paramsStream;
    samp::protocol::WriteSetVehicleParamsForPlayer(paramsStream, {404, 1, 0});
    Check(paramsStream.GetNumberOfBitsUsed() == 4 * 8, "vehicle parameters are four bytes");

    samp::protocol::SetVehicleParamsForPlayer params;
    Check(samp::protocol::ReadSetVehicleParamsForPlayer(paramsStream, params) &&
              params.value1 == 1 && params.value2 == 0,
          "both parameter bytes round-trip in order");

    samp::net::BitStream collisionStream;
    samp::protocol::WriteDisableVehicleCollisions(collisionStream, {true});
    Check(collisionStream.GetNumberOfBitsUsed() == 1,
          "the vehicle collision switch is a single bit");

    samp::protocol::DisableVehicleCollisions collisions;
    Check(samp::protocol::ReadDisableVehicleCollisions(collisionStream, collisions) &&
              collisions.disabled,
          "the collision switch round-trips");
}

void TestClassAndSpawnResponses() {
    samp::protocol::RequestClassResponse sent;
    sent.accepted = 1;
    sent.spawnInfo.team = 1;
    sent.spawnInfo.skin = 100;
    sent.spawnInfo.position = {5.0f, 6.0f, 7.0f};
    sent.spawnInfo.weapons = {24, samp::protocol::kNoWeapon, samp::protocol::kNoWeapon};

    samp::net::BitStream stream;
    samp::protocol::WriteRequestClassResponse(stream, sent);

    Check(stream.GetNumberOfBitsUsed() ==
              static_cast<int>(1 + samp::protocol::kSpawnInfoSize) * 8,
          "a class response is a verdict plus a spawn block");

    samp::protocol::RequestClassResponse received;
    Check(samp::protocol::ReadRequestClassResponse(stream, received), "class response reads back");
    Check(received.accepted == 1, "the verdict round-trips");
    Check(received.spawnInfo.skin == 100 && received.spawnInfo.position.z == 7.0f,
          "the shared spawn block round-trips inside another message");

    samp::net::BitStream spawnStream;
    samp::protocol::WriteRequestSpawnResponse(
        spawnStream, {static_cast<std::uint8_t>(samp::protocol::SpawnDecision::Immediate)});
    Check(spawnStream.GetNumberOfBitsUsed() == 8, "a spawn response is a single byte");

    samp::protocol::RequestSpawnResponse spawn;
    Check(samp::protocol::ReadRequestSpawnResponse(spawnStream, spawn) &&
              spawn.decision ==
                  static_cast<std::uint8_t>(samp::protocol::SpawnDecision::Immediate),
          "the spawn decision round-trips");
}

void TestSpecialActionDoorsAndStunts() {
    samp::net::BitStream actionStream;
    samp::protocol::WriteSetPlayerSpecialAction(actionStream, {12, 68});
    Check(actionStream.GetNumberOfBitsUsed() == 3 * 8, "a special action is three bytes");

    samp::protocol::SetPlayerSpecialAction action;
    Check(samp::protocol::ReadSetPlayerSpecialAction(actionStream, action) && action.action == 68,
          "the special action round-trips");

    samp::net::BitStream doorStream;
    samp::protocol::WriteSetVehicleDoors(doorStream, {404, 0x0B});
    samp::protocol::SetVehicleDoors doors;
    Check(samp::protocol::ReadSetVehicleDoors(doorStream, doors) && doors.doorStates == 0x0B,
          "the door bits round-trip");

    samp::net::BitStream stuntStream;
    samp::protocol::WriteEnableStuntBonus(stuntStream, {true});
    Check(stuntStream.GetNumberOfBitsUsed() == 1, "the stunt bonus switch is a single bit");

    samp::protocol::EnableStuntBonus stunt;
    Check(samp::protocol::ReadEnableStuntBonus(stuntStream, stunt) && stunt.enabled,
          "the stunt bonus switch round-trips");
}

void TestDiscardedBodyIsStillConsumed() {

    samp::net::BitStream stream;
    samp::protocol::WriteEmptyPacket(stream, {0xBEEF});
    stream.Write<std::uint32_t>(0x12345678);

    samp::protocol::EmptyPacket ignored;
    Check(samp::protocol::ReadEmptyPacket(stream, ignored), "the discarded body reads");
    Check(ignored.ignoredValue == 0xBEEF, "the value is still recoverable");

    std::uint32_t following = 0;
    Check(stream.Read(following) && following == 0x12345678,
          "whatever follows stays aligned because the body was consumed");
}

void TestObjectMovementMessages() {
    samp::protocol::ApplyObjectMovement sent;
    sent.objectId = 700;
    sent.values = {1, 2, 3, 4, 5};

    samp::net::BitStream stream;
    samp::protocol::WriteApplyObjectMovement(stream, sent);
    Check(stream.GetNumberOfBitsUsed() == 22 * 8, "a movement update is twenty-two bytes");

    samp::protocol::ApplyObjectMovement received;
    Check(samp::protocol::ReadApplyObjectMovement(stream, received), "movement update reads back");
    Check(received.objectId == 700, "the object id round-trips");
    Check(received.values == sent.values, "all five values keep their order");

    samp::net::BitStream rotationStream;
    samp::protocol::WriteApplyObjectTargetRotation(rotationStream, {700});
    Check(rotationStream.GetNumberOfBitsUsed() == 2 * 8,
          "applying the target rotation names an object and nothing else");

    samp::protocol::ApplyObjectTargetRotation rotation;
    Check(samp::protocol::ReadApplyObjectTargetRotation(rotationStream, rotation) &&
              rotation.objectId == 700,
          "the target rotation request round-trips");
}

}

int main() {
    TestDiscardedBodyIsStillConsumed();
    TestObjectMovementMessages();
    TestClassAndSpawnResponses();
    TestSpecialActionDoorsAndStunts();
    TestVehicleSeatingAndSkin();
    TestFindZAndVehicleParams();
    TestObjectControlMessages();
    TestWidescreenToggle();
    TestClientCheckRoundTrip();
    TestClientCheckBoundsAreChecked();
    TestObjectEditingMessages();
    TestAttachObjectToPlayer();
    TestShopNameIsFixedWidth();
    TestAudioStreamAndDrunkLevel();
    TestEmptyBodiedMessage();
    TestAttachedObjectRoundTrip();
    TestClearingAttachmentIsShorter();
    TestSmallMessages();
    TestActorAnimationRoundTrip();
    TestAnimationClearing();
    TestActorLifecycle();
    TestActorUpdates();
    TestInitGameRoundTrip();
    TestInitGameFlagsAreSingleBits();
    TestServerJoinAndQuit();
    TestConnectionRejected();
    TestScoreUpdateIsLengthDriven();
    TestScoreUpdateIgnoresTrailingScrap();
    TestWorldVehicleAddRoundTrip();
    TestVehicleMarkersAreRecognised();
    TestWorldPlayerAddRoundTrip();
    TestTeamlessPlayerIsRecognised();
    TestRaceCheckpointRoundTrip();
    TestChatBubbleRoundTrip();
    TestWorldPlayerEvents();
    Test3DTextLabelRoundTrip();
    TestCheckpointRoundTrip();
    TestSkillLevelRoundTrip();
    TestRemoveBuildingRoundTrip();
    TestDeathMessageWithAndWithoutKiller();
    TestSelectTextDrawIsBitPacked();
    TestMapIconRoundTrip();
    TestNumberPlateRoundTrip();
    TestTrailerMessages();
    TestVehicleComponentAndInterior();
    TestWorldBoundsOrder();
    TestSpawnInfoRoundTrip();
    TestSpawnWeaponsAreNotInterleaved();
    TestPlayerNameRoundTrip();
    TestPlayerNameLengthIsCapped();
    TestGameTextRoundTrip();
    TestEmptyGameTextIsRefused();
    TestPlayerTeamRoundTrip();
    TestTextDrawRoundTrip();
    TestTextDrawModelPreview();
    TestTextDrawHideAndUpdate();
    TestTextDrawLimitsDifferByOne();
    TestMoveObjectRoundTrip();
    TestTextureMaterialRoundTrip();
    TestTextMaterialRoundTrip();
    TestMaterialModelRulesDiffer();
    TestSetObjectMaterialReusesTheEntryFormat();
    TestOverlongNameStaysReadableButInvalid();
    TestMaterialListMatchesCount();
    TestUnknownMaterialKindIsRejected();
    TestFreeStandingObject();
    TestObjectAttachedToVehicle();
    TestObjectAttachedToObject();
    TestObjectMaterialCountIsCarried();
    TestWeaponMessages();
    TestSpectateMessages();
    TestSpectateFallbackDiffersByTarget();
    TestCompressedTextRoundTrip();
    TestDialogWithCompressedBody();
    TestTruncatedCompressedTextIsRejected();
    TestObjectAndPlayerToggles();
    TestPlaySoundRoundTrip();
    TestArmedWeaponIsValidated();
    TestCameraMessages();
    TestUnknownCutTypeFallsBack();
    TestVehiclePlacementMessages();
    TestPoolLimits();
    TestPlayerStateMessages();
    TestWorldStateMessages();
    TestPickupRoundTrip();
    TestPickupSlotIsBoundsChecked();
    TestSingleByteMessages();
    TestVehicleEntryRoundTrip();
    TestVehicleExitRoundTrip();
    TestDamageStatusRoundTrip();
    TestChatRoundTrip();
    TestEmptyStringRoundTrip();
    TestClientMessageRoundTrip();
    TestOversizedLengthIsRejected();
    TestTruncatedStringIsRejected();
    TestLongStringIsClamped();
    TestDialogHeaderRoundTrip();

    if (g_failures == 0) {
        std::printf("All RPC payload tests passed.\n");
        return 0;
    }

    std::printf("%d check(s) failed.\n", g_failures);
    return 1;
}
