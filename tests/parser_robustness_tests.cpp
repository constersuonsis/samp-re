#include "samp/protocol/packet_reader.h"
#include "samp/protocol/rpc_payloads.h"
#include "samp/protocol/sync_codec.h"

#include <cstdio>
#include <cstdint>
#include <functional>
#include <random>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void Check(bool condition, const std::string& what) {
    if (!condition) {
        std::printf("FAIL: %s\n", what.c_str());
        ++g_failures;
    }
}

/// A reader reduced to "consume this stream and say whether it parsed".
struct Parser {
    const char* name;
    std::function<bool(samp::net::BitStream&)> parse;
};

template <typename Message>
Parser Wrap(const char* name, bool (*read)(samp::net::BitStream&, Message&)) {
    return {name, [read](samp::net::BitStream& stream) {
                Message message;
                return read(stream, message);
            }};
}

/// Every reader that takes a stream and one out parameter.
std::vector<Parser> AllParsers() {
    using namespace samp::protocol;
    return {
        Wrap<ChatMessage>("ChatMessage", ReadChatMessage),
        Wrap<ClientMessage>("ClientMessage", ReadClientMessage),
        Wrap<DialogHeader>("DialogHeader", ReadDialogHeader),
        Wrap<EnterVehicle>("EnterVehicle", ReadEnterVehicle),
        Wrap<ExitVehicle>("ExitVehicle", ReadExitVehicle),
        Wrap<VehicleDamageStatus>("VehicleDamageStatus", ReadVehicleDamageStatus),
        Wrap<CreatePickup>("CreatePickup", ReadCreatePickup),
        Wrap<DestroyPickup>("DestroyPickup", ReadDestroyPickup),
        Wrap<SetWeather>("SetWeather", ReadSetWeather),
        Wrap<SetPlayerPosition>("SetPlayerPosition", ReadSetPlayerPosition),
        Wrap<SetPlayerHealth>("SetPlayerHealth", ReadSetPlayerHealth),
        Wrap<SetPlayerColour>("SetPlayerColour", ReadSetPlayerColour),
        Wrap<SetVehicleHealth>("SetVehicleHealth", ReadSetVehicleHealth),
        Wrap<SetPlayerCameraLookAt>("SetPlayerCameraLookAt", ReadSetPlayerCameraLookAt),
        Wrap<SetVehiclePosition>("SetVehiclePosition", ReadSetVehiclePosition),
        Wrap<CreateObject>("CreateObject", ReadCreateObject),
        Wrap<ObjectMaterial>("ObjectMaterial", ReadObjectMaterial),
        Wrap<SetObjectMaterial>("SetObjectMaterial", ReadSetObjectMaterial),
        Wrap<MoveObject>("MoveObject", ReadMoveObject),
        Wrap<ShowTextDraw>("ShowTextDraw", ReadShowTextDraw),
        Wrap<SetTextDrawString>("SetTextDrawString", ReadSetTextDrawString),
        Wrap<SetPlayerName>("SetPlayerName", ReadSetPlayerName),
        Wrap<DisplayGameText>("DisplayGameText", ReadDisplayGameText),
        Wrap<SpawnInfo>("SpawnInfo", ReadSpawnInfo),
        Wrap<SetPlayerMapIcon>("SetPlayerMapIcon", ReadSetPlayerMapIcon),
        Wrap<SetVehicleNumberPlate>("SetVehicleNumberPlate", ReadSetVehicleNumberPlate),
        Wrap<AttachTrailerToVehicle>("AttachTrailerToVehicle", ReadAttachTrailerToVehicle),
        Wrap<RemoveBuildingForPlayer>("RemoveBuildingForPlayer", ReadRemoveBuildingForPlayer),
        Wrap<DeathMessage>("DeathMessage", ReadDeathMessage),
        Wrap<SelectTextDraw>("SelectTextDraw", ReadSelectTextDraw),
        Wrap<Create3DTextLabel>("Create3DTextLabel", ReadCreate3DTextLabel),
        Wrap<SetCheckpoint>("SetCheckpoint", ReadSetCheckpoint),
        Wrap<SetRaceCheckpoint>("SetRaceCheckpoint", ReadSetRaceCheckpoint),
        Wrap<ChatBubble>("ChatBubble", ReadChatBubble),
        Wrap<WorldPlayerAdd>("WorldPlayerAdd", ReadWorldPlayerAdd),
        Wrap<WorldVehicleAdd>("WorldVehicleAdd", ReadWorldVehicleAdd),
        Wrap<ServerJoin>("ServerJoin", ReadServerJoin),
        Wrap<ConnectionRejected>("ConnectionRejected", ReadConnectionRejected),
        Wrap<InitGame>("InitGame", ReadInitGame),
        Wrap<ShowActor>("ShowActor", ReadShowActor),
        Wrap<ApplyActorAnimation>("ApplyActorAnimation", ReadApplyActorAnimation),
        Wrap<SetPlayerAttachedObject>("SetPlayerAttachedObject", ReadSetPlayerAttachedObject),
        Wrap<ClientCheckRequest>("ClientCheckRequest", ReadClientCheckRequest),
        Wrap<SetPlayerShopName>("SetPlayerShopName", ReadSetPlayerShopName),
        Wrap<PlayAudioStream>("PlayAudioStream", ReadPlayAudioStream),
        Wrap<EditObject>("EditObject", ReadEditObject),
        Wrap<AttachObjectToPlayer>("AttachObjectToPlayer", ReadAttachObjectToPlayer),
        Wrap<RequestClassResponse>("RequestClassResponse", ReadRequestClassResponse),
        Wrap<ApplyObjectMovement>("ApplyObjectMovement", ReadApplyObjectMovement),
        Wrap<PacketHeader>("PacketHeader", ReadPacketHeader),
    };
}

/// A reader must never claim success while having read past the end.
bool ParsedWithinBounds(const Parser& parser, const std::vector<std::uint8_t>& bytes) {
    samp::net::BitStream stream(bytes.data(), bytes.size(), false);

    const bool parsed = parser.parse(stream);
    if (!parsed) {
        return true;
    }
    return stream.GetReadOffset() <= stream.GetNumberOfBitsUsed();
}

void TestTruncatedInputIsSafe() {
    // Feed every reader every prefix length of a plausible packet. Each has to
    // either refuse or stay inside the buffer; neither may it read past the end.
    std::vector<std::uint8_t> full(96);
    for (std::size_t i = 0; i < full.size(); ++i) {
        full[i] = static_cast<std::uint8_t>(i * 3 + 1);
    }

    for (const Parser& parser : AllParsers()) {
        for (std::size_t length = 0; length <= full.size(); ++length) {
            const std::vector<std::uint8_t> prefix(full.begin(),
                                                   full.begin() + static_cast<long>(length));
            if (!ParsedWithinBounds(parser, prefix)) {
                Check(false, std::string(parser.name) + " overran a truncated buffer of " +
                                 std::to_string(length) + " bytes");
                break;
            }
        }
    }
}

void TestRandomInputIsSafe() {
    // Deterministic seed: a failure has to be reproducible.
    std::mt19937 generator(20240117);
    std::uniform_int_distribution<int> byteValue(0, 255);
    std::uniform_int_distribution<std::size_t> lengthValue(0, 160);

    const std::vector<Parser> parsers = AllParsers();

    for (int round = 0; round < 200; ++round) {
        std::vector<std::uint8_t> noise(lengthValue(generator));
        for (std::uint8_t& byte : noise) {
            byte = static_cast<std::uint8_t>(byteValue(generator));
        }

        for (const Parser& parser : parsers) {
            if (!ParsedWithinBounds(parser, noise)) {
                Check(false, std::string(parser.name) + " overran on random input in round " +
                                 std::to_string(round));
                return;
            }
        }
    }
}

void TestEmptyInputIsRefused() {
    // Nothing at all must not be mistaken for a valid message. The readers that
    // legitimately accept an empty body are not in this table.
    const std::vector<std::uint8_t> nothing;

    for (const Parser& parser : AllParsers()) {
        samp::net::BitStream stream(nothing.data(), nothing.size(), false);
        if (parser.parse(stream)) {
            Check(false, std::string(parser.name) + " accepted an empty packet");
        }
    }
}

}  // namespace

int main() {
    TestTruncatedInputIsSafe();
    TestRandomInputIsSafe();
    TestEmptyInputIsRefused();

    if (g_failures == 0) {
        std::printf("All parser robustness checks passed.\n");
        return 0;
    }

    std::printf("%d check(s) failed.\n", g_failures);
    return 1;
}
