#include "samp/protocol/rpc_dispatcher.h"

#include "samp/protocol/rpc_payloads.h"

#include <cstdio>
#include <string>

namespace {

int g_failures = 0;

void Check(bool condition, const char* what) {
    if (!condition) {
        std::printf("FAIL: %s\n", what);
        ++g_failures;
    }
}

void TestTypedDispatch() {
    samp::protocol::RpcDispatcher dispatcher;

    std::string seen;
    std::uint16_t seenId = 0;
    dispatcher.On<samp::protocol::ChatMessage>(
        samp::protocol::RpcId::Chat, samp::protocol::ReadChatMessage,
        [&](const samp::protocol::ChatMessage& message) {
            seen = message.text;
            seenId = message.playerId;
        });

    samp::net::BitStream payload;
    samp::protocol::WriteChatMessage(payload, {7, "hello"});

    Check(dispatcher.Dispatch(samp::protocol::RpcId::Chat, payload) ==
              samp::protocol::DispatchResult::Handled,
          "a bound id is handled");
    Check(seen == "hello" && seenId == 7, "the handler receives a parsed message");
}

void TestUnregisteredIsNotAnError() {
    samp::protocol::RpcDispatcher dispatcher;

    samp::net::BitStream payload;
    samp::protocol::WriteChatMessage(payload, {1, "ignored"});

    // A client is not obliged to answer every id, so this is normal.
    Check(dispatcher.Dispatch(samp::protocol::RpcId::Chat, payload) ==
              samp::protocol::DispatchResult::Unregistered,
          "an unbound id reports as unregistered, not as a failure");
    Check(!dispatcher.IsRegistered(samp::protocol::RpcId::Chat), "nothing is bound");
}

void TestMalformedIsDistinctFromUnregistered() {
    samp::protocol::RpcDispatcher dispatcher;

    bool ran = false;
    dispatcher.On<samp::protocol::ChatMessage>(
        samp::protocol::RpcId::Chat, samp::protocol::ReadChatMessage,
        [&](const samp::protocol::ChatMessage&) { ran = true; });

    // Claims eight bytes of text but supplies three.
    samp::net::BitStream payload;
    payload.Write<std::uint16_t>(1);
    payload.Write<std::uint8_t>(8);
    payload.WriteBytes("abc", 3);

    Check(dispatcher.Dispatch(samp::protocol::RpcId::Chat, payload) ==
              samp::protocol::DispatchResult::Malformed,
          "a bound id with a bad payload reports as malformed");
    Check(!ran, "the handler is not called with half-parsed data");
}

void TestEmptyBodiedBinding() {
    samp::protocol::RpcDispatcher dispatcher;

    int restarts = 0;
    dispatcher.On(samp::protocol::RpcId::GameModeRestart, [&] { ++restarts; });

    // Nothing follows the id, and that has to be enough.
    samp::net::BitStream empty;
    Check(dispatcher.Dispatch(samp::protocol::RpcId::GameModeRestart, empty) ==
              samp::protocol::DispatchResult::Handled,
          "an id with no body dispatches on an empty payload");
    Check(restarts == 1, "the no-argument handler ran");
}

void TestRebindingReplaces() {
    samp::protocol::RpcDispatcher dispatcher;

    int first = 0;
    int second = 0;
    dispatcher.On(samp::protocol::RpcId::GameModeRestart, [&] { ++first; });
    dispatcher.On(samp::protocol::RpcId::GameModeRestart, [&] { ++second; });

    samp::net::BitStream empty;
    dispatcher.Dispatch(samp::protocol::RpcId::GameModeRestart, empty);

    Check(first == 0 && second == 1, "rebinding an id replaces the previous handler");
    Check(dispatcher.Size() == 1, "rebinding does not add a second entry");
}

void TestSeveralIdsCoexist() {
    samp::protocol::RpcDispatcher dispatcher;

    std::uint8_t weather = 0;
    float gravity = 0.0f;

    dispatcher.On<samp::protocol::SetWeather>(
        samp::protocol::RpcId::SetWeather, samp::protocol::ReadSetWeather,
        [&](const samp::protocol::SetWeather& message) { weather = message.weatherId; });
    dispatcher.On<samp::protocol::SetGravity>(
        samp::protocol::RpcId::SetGravity, samp::protocol::ReadSetGravity,
        [&](const samp::protocol::SetGravity& message) { gravity = message.gravity; });

    samp::net::BitStream weatherPayload;
    samp::protocol::WriteSetWeather(weatherPayload, {9});
    samp::net::BitStream gravityPayload;
    samp::protocol::WriteSetGravity(gravityPayload, {0.005f});

    dispatcher.Dispatch(samp::protocol::RpcId::SetWeather, weatherPayload);
    dispatcher.Dispatch(samp::protocol::RpcId::SetGravity, gravityPayload);

    Check(weather == 9 && gravity == 0.005f, "each id reaches its own handler");
    Check(dispatcher.Size() == 2, "both bindings are held");

    dispatcher.Clear();
    Check(dispatcher.Size() == 0, "clearing drops every binding");
}

}  // namespace

int main() {
    TestTypedDispatch();
    TestUnregisteredIsNotAnError();
    TestMalformedIsDistinctFromUnregistered();
    TestEmptyBodiedBinding();
    TestRebindingReplaces();
    TestSeveralIdsCoexist();

    if (g_failures == 0) {
        std::printf("All dispatcher tests passed.\n");
        return 0;
    }

    std::printf("%d check(s) failed.\n", g_failures);
    return 1;
}
