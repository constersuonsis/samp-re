#include "samp/client/network_client.h"

#include "PacketEnumerations.h"
#include "RakClientInterface.h"
#include "RakNetworkFactory.h"
#include "SHA1.h"
#include "samp/client/security_archive.h"
#include "samp/client/spawn.h"
#include "samp/game/memory.h"
#include "samp/util/logger.h"

#include <windows.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

namespace samp::client {
namespace {

enum class NetworkState {
  Connecting = 1,
  WaitingForResponse = 2,
  Active = 5,
  JoiningGame = 6
};

struct ServerGameSettings {
  std::array<bool, 11> flags{};
  std::array<std::uint32_t, 11> values{};
  std::uint16_t player_id = 0;
  std::array<std::uint8_t, 2> byte_values{};
  std::string hostname;
  std::array<char, kServerModelBlockSize> model_data{};
  std::array<bool, kServerModelBlockSize> loaded_models{};
  std::uint32_t final_value = 0;
};

struct PlayerSyncState {
  std::uint16_t left_right_analog = 0;
  std::uint16_t up_down_analog = 0;
  std::uint16_t keys = 0;
  std::array<float, 3> position{};
  std::array<float, 4> rotation{};
  std::uint8_t health = 0;
  std::uint8_t armor = 0;
  std::uint8_t special_action = 0;
  std::uint8_t weapon = 0;
  std::array<float, 3> velocity{};
  std::uint16_t surfing_vehicle = 0xFFFF;
  std::array<float, 3> surfing_offset{};
  std::uint32_t animation_flags = 0;
  bool has_surfing = false;
  bool has_animation = false;
};

struct VehicleSyncState {
  std::uint16_t vehicle_id = 0;
  std::uint16_t left_right_analog = 0;
  std::uint16_t up_down_analog = 0;
  std::uint16_t keys = 0;
  std::array<float, 4> rotation{};
  std::array<float, 3> position{};
  std::array<float, 3> velocity{};
  std::uint16_t trailer_id = 0;
  std::uint8_t health = 0;
  std::uint8_t armor = 0;
  std::uint8_t damage_status = 0;
  std::uint16_t vehicle_flags = 0;
  std::uint32_t train_speed = 0;
  bool has_siren = false;
  bool has_landing_gear = false;
};

struct RemotePlayerSpawnState {
  std::uint8_t skin_id = 0xFF;
  std::uint8_t special_action = 4;
  std::uint32_t spawn_model_parameter = 0;
  std::array<float, 3> position{};
  float rotation = 0.0f;
  std::uint32_t player_color = 0;
  std::array<std::uint16_t, 11> equipment{};
};

struct RemotePlayer {
  std::array<char, 25> name{};
  PlayerSyncState sync_state{};
  VehicleSyncState vehicle_sync_state{};
  RemotePlayerSpawnState spawn_state{};
  std::uint32_t last_sync_time = 0;
  std::uint32_t last_vehicle_sync_time = 0;
  std::uint32_t sync_packet_count = 0;
  std::uint32_t vehicle_sync_packet_count = 0;
  std::uint32_t color = 0;
  bool active = false;
  bool npc = false;
  bool has_sync_state = false;
  bool has_vehicle_sync_state = false;
  bool has_spawn_state = false;
  bool world_spawned = false;
};

struct ClientDeleter {
  void operator()(RakClientInterface *client) const {
    if (client) {
      client->Disconnect(0);
      RakNetworkFactory::DestroyRakClientInterface(client);
    }
  }
};

struct NetworkClient {
  std::unique_ptr<RakClientInterface, ClientDeleter> peer;
  std::string host;
  std::string name;
  std::string password;
  unsigned short port = 0;
  DWORD last_attempt = 0;
  std::uint32_t server_challenge = 0;
  std::string server_auth_key;
  ServerGameSettings server_game_settings;
  std::array<RemotePlayer, 1005> remote_players{};
  bool has_server_game_settings = false;
  NetworkState state = NetworkState::Connecting;
};

std::unique_ptr<NetworkClient> g_client;
int g_init_game_rpc_id = 139;
int g_connection_rejected_rpc_id = 130;
int g_client_message_rpc_id = 93;
int g_server_join_rpc_id = 137;
int g_server_quit_rpc_id = 138;
int g_server_weather_rpc_id = 152;
int g_world_player_add_rpc_id = 32;
int g_world_player_remove_rpc_id = 163;
constexpr std::size_t kStuntBonusFlagIndex = 4;
constexpr std::size_t kGravityValueIndex = 4;
constexpr std::size_t kWeatherValueIndex = 1;

bool ReadConnectionAcceptance(Packet &packet, unsigned offset,
                              std::uint32_t &server_challenge) {
  if (packet.length <= offset) {
    return false;
  }
  RakNet::BitStream stream(packet.data + offset, packet.length - offset, false);
  unsigned char message_id = 0;
  unsigned int remote_address = 0;
  unsigned short remote_port = 0;
  unsigned short player_index = 0;
  if (!stream.Read(message_id) || message_id != ID_CONNECTION_REQUEST_ACCEPTED ||
      !stream.Read(remote_address) || !stream.Read(remote_port) || !stream.Read(player_index) ||
      !stream.Read(server_challenge)) {
    return false;
  }
  (void)remote_address;
  (void)remote_port;
  (void)player_index;
  server_challenge ^= 0xFD9;
  return true;
}

bool ReadAuthenticationKey(Packet &packet, unsigned offset, std::string &auth_key) {
  if (packet.length <= offset) {
    return false;
  }
  RakNet::BitStream stream(packet.data + offset, packet.length - offset, false);
  unsigned char message_id = 0;
  unsigned char key_length = 0;
  if (!stream.Read(message_id) || message_id != ID_AUTH_KEY || !stream.Read(key_length) ||
      stream.GetNumberOfUnreadBits() < key_length * 8) {
    return false;
  }
  std::array<char, 256> key_bytes{};
  if (!stream.Read(key_bytes.data(), key_length)) {
    return false;
  }
  auth_key.assign(key_bytes.data(), key_length);
  return true;
}

bool GenerateJoinKey(char *key, size_t capacity) {
  auto *path = reinterpret_cast<unsigned char *>(0xC9236C);
  const size_t path_length = strnlen_s(reinterpret_cast<const char *>(path), 256);
  if (path_length == 256 || capacity < 2) {
    return false;
  }

  CSHA1 sha1;
  sha1.Update(path, static_cast<unsigned int>(path_length));
  sha1.Final();
  const unsigned char *digest = sha1.GetHash();
  std::array<std::uint32_t, 6> limbs{};
  for (size_t index = 0; index < SHA1_LENGTH; ++index) {
    const size_t word_offset = index & ~static_cast<size_t>(3);
    const unsigned char value = digest[word_offset + (3 - (index & 3))];
    const unsigned char first = value & 3;
    const unsigned char second = (value >> 2) & 3;
    const unsigned char third = (value >> 4) & 3;
    const unsigned char fourth = value >> 6;
    const unsigned char low_pair = first <= second ? first | (second << 2)
                                                   : second | (first << 2);
    const unsigned char high_pair = third <= fourth ? third | (fourth << 2)
                                                     : fourth | (third << 2);
    const unsigned char encoded = low_pair | (high_pair << 4);
    std::uint64_t carry = encoded;
    for (std::uint32_t &limb : limbs) {
      const std::uint64_t value_with_carry =
          (static_cast<std::uint64_t>(limb) << 8) | carry;
      limb = static_cast<std::uint32_t>(value_with_carry);
      carry = value_with_carry >> 32;
    }
    if (carry != 0) {
      return false;
    }
  }

  std::uint64_t carry = 0;
  for (std::uint32_t &limb : limbs) {
    const std::uint64_t product = static_cast<std::uint64_t>(limb) * 1001 + carry;
    limb = static_cast<std::uint32_t>(product);
    carry = product >> 32;
  }
  if (carry != 0) {
    return false;
  }

  static constexpr char digits[] = "0123456789ABCDEF";
  size_t output_length = 0;
  bool started = false;
  for (size_t limb_index = limbs.size(); limb_index-- > 0;) {
    for (int shift = 28; shift >= 0; shift -= 4) {
      const unsigned char digit = static_cast<unsigned char>((limbs[limb_index] >> shift) & 0xF);
      if (!started && digit == 0) {
        continue;
      }
      if (output_length + 1 >= capacity) {
        return false;
      }
      key[output_length++] = digits[digit];
      started = true;
    }
  }
  if (!started) {
    key[output_length++] = '0';
  }
  key[output_length] = 0;
  return true;
}

bool SendJoinRequest(NetworkClient &client) {
  char key[64] = {};
  if (!GenerateJoinKey(key, sizeof(key)) || client.name.size() > 255) {
    return false;
  }

  constexpr char version[] = "0.3.7-R3";
  const auto name_length = static_cast<unsigned char>(client.name.size());
  const auto key_length = static_cast<unsigned char>(std::strlen(key));
  const auto version_length = static_cast<unsigned char>(sizeof(version) - 1);
  RakNet::BitStream payload;
  payload.Write(static_cast<std::uint32_t>(0xFD9));
  payload.Write(static_cast<unsigned char>(1));
  payload.Write(name_length);
  payload.Write(client.name.data(), static_cast<int>(client.name.size()));
  payload.Write(client.server_challenge);
  payload.Write(key_length);
  payload.Write(key, key_length);
  payload.Write(version_length);
  payload.Write(version, version_length);
  payload.Write(client.server_challenge);

  int rpc_id = 25;
  client.state = NetworkState::JoiningGame;
  return client.peer->RPC(&rpc_id, &payload, HIGH_PRIORITY, RELIABLE, 0, false,
                          UNASSIGNED_NETWORK_ID, nullptr);
}

bool SendAuthenticationKeyResponse(NetworkClient &client, const std::string &server_auth_key) {
  std::string hardware_id;
  if (!GenerateHardwareId(server_auth_key.c_str(), hardware_id) || hardware_id.size() > 255) {
    return false;
  }
  RakNet::BitStream payload;
  payload.Write(static_cast<unsigned char>(ID_AUTH_KEY));
  payload.Write(static_cast<unsigned char>(hardware_id.size()));
  payload.Write(hardware_id.data(), static_cast<int>(hardware_id.size()));
  return client.peer->Send(&payload, SYSTEM_PRIORITY, RELIABLE, 0);
}

bool ReadServerGameSettings(RakNet::BitStream &payload, unsigned int payload_bits,
                            ServerGameSettings &settings) {
  ServerGameSettings parsed;
  std::size_t flag_index = 0;
  std::size_t value_index = 0;
  auto read_flag = [&]() {
    return flag_index < parsed.flags.size() && payload.Read(parsed.flags[flag_index++]);
  };
  auto read_value = [&]() {
    return value_index < parsed.values.size() && payload.Read(parsed.values[value_index++]);
  };

  if (!read_flag() || !read_flag() || !read_flag() || !read_flag() || !read_value() ||
      !read_flag() || !read_value() || !read_flag() || !read_flag() || !read_flag() ||
      !read_value() || !payload.Read(parsed.player_id) || !read_flag() ||
      !read_value() || !payload.Read(parsed.byte_values[0]) || !payload.Read(parsed.byte_values[1]) ||
      !read_value() || !read_flag() || !read_value() || !read_flag()) {
    return false;
  }

  for (std::size_t index = 0; index < 5; ++index) {
    if (!read_value()) {
      return false;
    }
  }

  std::uint8_t hostname_length = 0;
  if (!payload.Read(hostname_length)) {
    return false;
  }
  const std::size_t required_payload_bits =
      parsed.flags.size() + parsed.values.size() * 32 + sizeof(parsed.player_id) * 8 +
      parsed.byte_values.size() * 8 + sizeof(hostname_length) * 8 +
      static_cast<std::size_t>(hostname_length) * 8 + parsed.model_data.size() * 8 +
      sizeof(parsed.final_value) * 8;
  if (static_cast<std::size_t>(payload_bits) < required_payload_bits) {
    return false;
  }
  const std::size_t required_bits =
      (static_cast<std::size_t>(hostname_length) + parsed.model_data.size() + sizeof(parsed.final_value)) * 8;
  if (static_cast<std::size_t>(payload.GetNumberOfUnreadBits()) < required_bits) {
    return false;
  }
  parsed.hostname.resize(hostname_length);
  if ((hostname_length != 0 && !payload.Read(parsed.hostname.data(), hostname_length)) ||
      !payload.Read(parsed.model_data.data(), static_cast<int>(parsed.model_data.size())) ||
      !payload.Read(parsed.final_value)) {
    return false;
  }
  settings = std::move(parsed);
  return true;
}

void ApplyServerGameOptions(const ServerGameSettings &settings) {
  samp::game::WriteByte(reinterpret_cast<void *>(0xA4A474),
                        settings.flags[kStuntBonusFlagIndex] ? 1 : 0);
  samp::game::WriteDword(reinterpret_cast<void *>(0x863984), settings.values[kGravityValueIndex]);
  ApplyServerWeather(settings.byte_values[kWeatherValueIndex]);
}

void HandleInitGameRpc(RPCParameters *parameters) {
  if (!parameters || !g_client || !parameters->input) {
    return;
  }
  const unsigned int payload_size = (parameters->numberOfBitsOfData + 7) / 8;
  RakNet::BitStream payload(parameters->input, payload_size, false);
  if (!ReadServerGameSettings(payload, parameters->numberOfBitsOfData,
                              g_client->server_game_settings)) {
    samp::util::WriteLogLine("network InitGame RPC rejected: truncated or invalid payload");
    return;
  }
  g_client->has_server_game_settings = true;
  const std::size_t requested_models = ApplyServerModelSettings(
      g_client->server_game_settings.model_data, g_client->server_game_settings.loaded_models);
  ApplyServerGameOptions(g_client->server_game_settings);
  samp::util::WriteLogNumber("network server models requested",
                             static_cast<unsigned>(requested_models));
  g_client->state = NetworkState::Active;
  samp::util::WriteLogLine("network InitGame completed; client state is active");
}

void HandleConnectionRejectedRpc(RPCParameters *parameters) {
  if (!parameters || !g_client || !parameters->input || parameters->numberOfBitsOfData < 8) {
    return;
  }
  RakNet::BitStream payload(parameters->input, (parameters->numberOfBitsOfData + 7) / 8, false);
  unsigned char reason = 0;
  if (!payload.Read(reason)) {
    return;
  }
  switch (reason) {
    case 1:
      samp::util::WriteLogLine("network connection rejected: incorrect version");
      break;
    case 2:
      samp::util::WriteLogLine("network connection rejected: unacceptable nickname");
      samp::util::WriteLogLine("network nickname must be 3-20 alphanumeric characters");
      break;
    case 3:
      samp::util::WriteLogLine("network connection rejected: incompatible client version");
      break;
    case 4:
      samp::util::WriteLogLine("network connection rejected: no player slot available");
      break;
    default:
      samp::util::WriteLogNumber("network connection rejected: unknown reason", reason);
      break;
  }
  g_client->peer->Disconnect(500);
}

void HandleClientMessageRpc(RPCParameters *parameters) {
  if (!parameters || !parameters->input || parameters->numberOfBitsOfData < 64) {
    return;
  }
  RakNet::BitStream payload(parameters->input, (parameters->numberOfBitsOfData + 7) / 8, false);
  std::uint32_t color = 0;
  std::uint32_t message_length = 0;
  if (!payload.Read(color) || !payload.Read(message_length) || message_length > 255 ||
      payload.GetNumberOfUnreadBits() < static_cast<int>(message_length * 8)) {
    samp::util::WriteLogLine("network client message RPC rejected: invalid payload");
    return;
  }
  std::array<char, 256> message{};
  if (message_length != 0 && !payload.Read(message.data(), static_cast<int>(message_length))) {
    samp::util::WriteLogLine("network client message RPC rejected: truncated text");
    return;
  }
  samp::util::WriteLogNumber("network client message color", color);
  samp::util::WriteLogValue("network client message", message.data());
}

void HandleServerJoinRpc(RPCParameters *parameters) {
  if (!parameters || !g_client || !parameters->input || parameters->numberOfBitsOfData < 48) {
    return;
  }
  RakNet::BitStream payload(parameters->input, (parameters->numberOfBitsOfData + 7) / 8, false);
  std::uint16_t player_id = 0;
  std::uint32_t color = 0;
  std::uint8_t npc_flag = 0;
  std::uint8_t name_length = 0;
  if (!payload.Read(player_id) || !payload.Read(color) || !payload.Read(npc_flag) ||
      !payload.Read(name_length) || payload.GetNumberOfUnreadBits() < static_cast<int>(name_length * 8)) {
    samp::util::WriteLogLine("network server join RPC rejected: invalid payload");
    return;
  }
  std::array<char, 256> name{};
  if (name_length != 0 && !payload.Read(name.data(), name_length)) {
    samp::util::WriteLogLine("network server join RPC rejected: truncated name");
    return;
  }
  if (player_id >= g_client->remote_players.size() || std::strlen(name.data()) > 24) {
    samp::util::WriteLogLine("network server join RPC rejected: player id or name is invalid");
    return;
  }
  RemotePlayer &player = g_client->remote_players[player_id];
  player.name.fill(0);
  std::memcpy(player.name.data(), name.data(), std::strlen(name.data()));
  player.color = color;
  player.active = true;
  player.npc = npc_flag != 0;
  samp::util::WriteLogNumber("network remote player joined", player_id);
  samp::util::WriteLogValue("network remote player name", player.name.data());
}

void HandleServerQuitRpc(RPCParameters *parameters) {
  if (!parameters || !g_client || !parameters->input || parameters->numberOfBitsOfData < 24) {
    return;
  }
  RakNet::BitStream payload(parameters->input, (parameters->numberOfBitsOfData + 7) / 8, false);
  std::uint16_t player_id = 0;
  std::uint8_t reason = 0;
  if (!payload.Read(player_id) || !payload.Read(reason)) {
    samp::util::WriteLogLine("network server quit RPC rejected: invalid payload");
    return;
  }
  if (player_id >= g_client->remote_players.size()) {
    samp::util::WriteLogNumber("network server quit RPC rejected: invalid player id", player_id);
    return;
  }
  g_client->remote_players[player_id] = {};
  samp::util::WriteLogNumber("network remote player left", player_id);
  samp::util::WriteLogNumber("network remote player quit reason", reason);
}

void HandleServerWeatherRpc(RPCParameters *parameters) {
  if (!parameters || !g_client || !parameters->input || parameters->numberOfBitsOfData < 8) {
    return;
  }
  RakNet::BitStream payload(parameters->input, (parameters->numberOfBitsOfData + 7) / 8, false);
  std::uint8_t weather = 0;
  if (!payload.Read(weather)) {
    samp::util::WriteLogLine("network weather RPC rejected: invalid payload");
    return;
  }
  g_client->server_game_settings.byte_values[kWeatherValueIndex] = weather;
  ApplyServerWeather(weather);
  samp::util::WriteLogNumber("network server weather updated", weather);
}

void HandleWorldPlayerAddRpc(RPCParameters *parameters) {
  constexpr int kSpawnPayloadBits = 400;
  if (!parameters || !g_client || !parameters->input ||
      parameters->numberOfBitsOfData < kSpawnPayloadBits) {
    samp::util::WriteLogLine("network world player add RPC rejected: truncated payload");
    return;
  }
  RakNet::BitStream payload(parameters->input, (parameters->numberOfBitsOfData + 7) / 8, false);
  std::uint16_t player_id = 0;
  RemotePlayerSpawnState spawn_state;
  if (!payload.Read(player_id) || !payload.Read(spawn_state.skin_id) ||
      !payload.Read(spawn_state.spawn_model_parameter) ||
      !payload.Read(reinterpret_cast<char *>(spawn_state.position.data()), 12) ||
      !payload.Read(spawn_state.rotation) || !payload.Read(spawn_state.player_color) ||
      !payload.Read(spawn_state.special_action) ||
      !payload.Read(reinterpret_cast<char *>(spawn_state.equipment.data()), 22)) {
    samp::util::WriteLogLine("network world player add RPC rejected: invalid payload");
    return;
  }
  if (player_id >= 0x3EC || player_id >= g_client->remote_players.size()) {
    samp::util::WriteLogNumber("network world player add RPC rejected: invalid player id", player_id);
    return;
  }
  RemotePlayer &player = g_client->remote_players[player_id];
  if (!player.active) {
    return;
  }
  player.spawn_state = spawn_state;
  player.has_spawn_state = true;
  player.world_spawned = true;
  player.has_sync_state = false;
  player.has_vehicle_sync_state = false;
  samp::util::WriteLogNumber("network remote player spawn received", player_id);
}

void HandleWorldPlayerRemoveRpc(RPCParameters *parameters) {
  if (!parameters || !g_client || !parameters->input || parameters->numberOfBitsOfData < 16) {
    samp::util::WriteLogLine("network world player remove RPC rejected: truncated payload");
    return;
  }
  RakNet::BitStream payload(parameters->input, (parameters->numberOfBitsOfData + 7) / 8, false);
  std::uint16_t player_id = 0;
  if (!payload.Read(player_id) || player_id > 0x3EC ||
      player_id >= g_client->remote_players.size()) {
    samp::util::WriteLogLine("network world player remove RPC rejected: invalid player id");
    return;
  }
  RemotePlayer &player = g_client->remote_players[player_id];
  player.world_spawned = false;
  player.has_spawn_state = false;
  player.has_sync_state = false;
  player.has_vehicle_sync_state = false;
  samp::util::WriteLogNumber("network remote player despawn received", player_id);
}

bool ReadRotationQuaternion(RakNet::BitStream &payload, std::array<float, 4> &rotation) {
  bool negative_w = false;
  bool negative_x = false;
  bool negative_y = false;
  bool negative_z = false;
  std::uint16_t encoded_x = 0;
  std::uint16_t encoded_y = 0;
  std::uint16_t encoded_z = 0;
  if (!payload.Read(negative_w) || !payload.Read(negative_x) || !payload.Read(negative_y) ||
      !payload.Read(negative_z) || !payload.Read(encoded_x) || !payload.Read(encoded_y) ||
      !payload.Read(encoded_z)) {
    return false;
  }
  constexpr double kQuaternionScale = 0.00001525902189669642;
  rotation[1] = static_cast<float>(encoded_x * kQuaternionScale);
  rotation[2] = static_cast<float>(encoded_y * kQuaternionScale);
  rotation[3] = static_cast<float>(encoded_z * kQuaternionScale);
  if (negative_x) {
    rotation[1] = -rotation[1];
  }
  if (negative_y) {
    rotation[2] = -rotation[2];
  }
  if (negative_z) {
    rotation[3] = -rotation[3];
  }
  float remaining = 1.0f - rotation[1] * rotation[1] - rotation[2] * rotation[2] -
                    rotation[3] * rotation[3];
  if (remaining < 0.0f) {
    remaining = 0.0f;
  }
  rotation[0] = std::sqrt(remaining);
  if (negative_w) {
    rotation[0] = -rotation[0];
  }
  return true;
}

bool ReadPlayerVelocity(RakNet::BitStream &payload, std::array<float, 3> &velocity) {
  float magnitude = 0.0f;
  if (!payload.Read(magnitude)) {
    return false;
  }
  if (magnitude <= 0.00001f) {
    velocity.fill(0.0f);
    return true;
  }
  for (float &component : velocity) {
    std::uint16_t encoded_component = 0;
    if (!payload.Read(encoded_component)) {
      return false;
    }
    component = static_cast<float>(magnitude * (encoded_component * 0.000030518044 - 1.0));
  }
  return true;
}

bool ReadPlayerSyncState(RakNet::BitStream &payload, PlayerSyncState &state) {
  PlayerSyncState parsed;
  bool has_left_right_analog = false;
  bool has_up_down_analog = false;
  if (!payload.Read(has_left_right_analog) ||
      (has_left_right_analog && !payload.Read(parsed.left_right_analog)) ||
      !payload.Read(has_up_down_analog) ||
      (has_up_down_analog && !payload.Read(parsed.up_down_analog)) ||
      !payload.Read(parsed.keys) ||
      !payload.Read(reinterpret_cast<char *>(parsed.position.data()), 12) ||
      !ReadRotationQuaternion(payload, parsed.rotation)) {
    return false;
  }
  std::uint8_t health_armor = 0;
  std::uint8_t special_action = 0;
  if (!payload.Read(health_armor) || !payload.Read(special_action) ||
      !payload.Read(parsed.weapon) || !ReadPlayerVelocity(payload, parsed.velocity)) {
    return false;
  }
  parsed.health = (health_armor & 0x0F) == 0x0F ? 100 : (health_armor & 0x0F) * 7;
  parsed.armor = (health_armor >> 4) == 0x0F ? 100 : (health_armor >> 4) * 7;
  parsed.special_action = special_action & 0x3F;
  if (!payload.Read(parsed.has_surfing)) {
    return false;
  }
  if (parsed.has_surfing &&
      (!payload.Read(parsed.surfing_vehicle) ||
       !payload.Read(reinterpret_cast<char *>(parsed.surfing_offset.data()), 12))) {
    return false;
  }
  if (!payload.Read(parsed.has_animation)) {
    return false;
  }
  if (parsed.has_animation && !payload.Read(parsed.animation_flags)) {
    return false;
  }
  state = parsed;
  return true;
}

bool ReadVehicleSyncState(RakNet::BitStream &payload, VehicleSyncState &state) {
  VehicleSyncState parsed;
  std::uint8_t health_armor = 0;
  bool has_train_speed = false;
  bool has_vehicle_flags = false;
  if (!payload.Read(parsed.vehicle_id) || !payload.Read(parsed.left_right_analog) ||
      !payload.Read(parsed.up_down_analog) || !payload.Read(parsed.keys) ||
      !ReadRotationQuaternion(payload, parsed.rotation) ||
      !payload.Read(reinterpret_cast<char *>(parsed.position.data()), 12) ||
      !ReadPlayerVelocity(payload, parsed.velocity) || !payload.Read(parsed.trailer_id) ||
      !payload.Read(health_armor) || !payload.Read(parsed.damage_status) ||
      !payload.Read(parsed.has_siren) || !payload.Read(parsed.has_landing_gear) ||
      !payload.Read(has_train_speed)) {
    return false;
  }
  if (has_train_speed && !payload.Read(parsed.train_speed)) {
    return false;
  }
  if (!payload.Read(has_vehicle_flags) || (has_vehicle_flags && !payload.Read(parsed.vehicle_flags))) {
    return false;
  }
  parsed.health = (health_armor & 0x0F) == 0x0F ? 100 : (health_armor & 0x0F) * 7;
  parsed.armor = (health_armor >> 4) == 0x0F ? 100 : (health_armor >> 4) * 7;
  parsed.damage_status &= 0x3F;
  state = parsed;
  return true;
}

void HandlePlayerSyncPacket(Packet &packet, unsigned offset, std::uint32_t packet_timestamp) {
  if (!g_client || g_client->state != NetworkState::Active || packet.length - offset < 3) {
    return;
  }
  RakNet::BitStream payload(packet.data + offset, packet.length - offset, false);
  unsigned char packet_id = 0;
  std::uint16_t player_id = 0;
  if (!payload.Read(packet_id) || packet_id != ID_PLAYER_SYNC || !payload.Read(player_id) ||
      player_id >= g_client->remote_players.size()) {
    return;
  }
  RemotePlayer &player = g_client->remote_players[player_id];
  if (!player.active) {
    return;
  }
  PlayerSyncState sync_state;
  if (!ReadPlayerSyncState(payload, sync_state)) {
    samp::util::WriteLogNumber("network player sync packet rejected: malformed", player_id);
    return;
  }
  if (packet_timestamp != 0 && player.has_sync_state &&
      static_cast<std::int32_t>(packet_timestamp - player.last_sync_time) < 0) {
    return;
  }
  player.sync_state = sync_state;
  player.last_sync_time = packet_timestamp;
  player.has_sync_state = true;
  ++player.sync_packet_count;
}

void HandleVehicleSyncPacket(Packet &packet, unsigned offset, std::uint32_t packet_timestamp) {
  if (!g_client || g_client->state != NetworkState::Active || packet.length - offset < 3) {
    return;
  }
  RakNet::BitStream payload(packet.data + offset, packet.length - offset, false);
  unsigned char packet_id = 0;
  std::uint16_t player_id = 0;
  if (!payload.Read(packet_id) || packet_id != ID_VEHICLE_SYNC || !payload.Read(player_id) ||
      player_id >= g_client->remote_players.size()) {
    return;
  }
  RemotePlayer &player = g_client->remote_players[player_id];
  if (!player.active) {
    return;
  }
  VehicleSyncState sync_state;
  if (!ReadVehicleSyncState(payload, sync_state)) {
    samp::util::WriteLogNumber("network vehicle sync packet rejected: malformed", player_id);
    return;
  }
  if (packet_timestamp != 0 && player.has_vehicle_sync_state &&
      static_cast<std::int32_t>(packet_timestamp - player.last_vehicle_sync_time) < 0) {
    return;
  }
  player.vehicle_sync_state = sync_state;
  player.last_vehicle_sync_time = packet_timestamp;
  player.has_vehicle_sync_state = true;
  ++player.vehicle_sync_packet_count;
}

}

bool CreateNetworkClient(const char *host, unsigned short port, const char *name,
                         const char *password) {
  if (g_client) {
    return true;
  }
  if (!host || !host[0] || !name || !name[0] || !port) {
    return false;
  }
  auto client = std::make_unique<NetworkClient>();
  client->peer.reset(RakNetworkFactory::GetRakClientInterface());
  if (!client->peer) {
    return false;
  }
  client->host = host;
  client->name = name;
  client->password = password ? password : "";
  client->port = port;
  client->last_attempt = ::GetTickCount();
  client->peer->SetPassword(client->password.c_str());
  client->peer->RegisterAsRemoteProcedureCall(&g_init_game_rpc_id, &HandleInitGameRpc);
  client->peer->RegisterAsRemoteProcedureCall(&g_connection_rejected_rpc_id,
                                               &HandleConnectionRejectedRpc);
  client->peer->RegisterAsRemoteProcedureCall(&g_client_message_rpc_id, &HandleClientMessageRpc);
  client->peer->RegisterAsRemoteProcedureCall(&g_server_join_rpc_id, &HandleServerJoinRpc);
  client->peer->RegisterAsRemoteProcedureCall(&g_server_quit_rpc_id, &HandleServerQuitRpc);
  client->peer->RegisterAsRemoteProcedureCall(&g_server_weather_rpc_id, &HandleServerWeatherRpc);
  client->peer->RegisterAsRemoteProcedureCall(&g_world_player_add_rpc_id,
                                               &HandleWorldPlayerAddRpc);
  client->peer->RegisterAsRemoteProcedureCall(&g_world_player_remove_rpc_id,
                                               &HandleWorldPlayerRemoveRpc);
  g_client = std::move(client);
  return true;
}

bool HasNetworkClient() {
  return g_client != nullptr;
}

int NetworkClientState() {
  return g_client ? static_cast<int>(g_client->state) : 0;
}

void TryConnectNetworkClient() {
  if (!g_client || g_client->state != NetworkState::Connecting) {
    return;
  }
  DWORD now = ::GetTickCount();
  if (now - g_client->last_attempt <= 3000) {
    return;
  }
  g_client->peer->Connect(g_client->host.c_str(), g_client->port, 0, 0, 2, nullptr);
  g_client->last_attempt = now;
  g_client->state = NetworkState::WaitingForResponse;
}

void PumpNetworkClient() {
  if (!g_client) {
    return;
  }
  RakClientInterface &peer = *g_client->peer;
  while (Packet *packet = peer.Receive()) {
    unsigned offset = 0;
    std::uint32_t packet_timestamp = 0;
    if (packet->length > sizeof(RakNetTime) + 1 && packet->data[0] == ID_TIMESTAMP) {
      std::memcpy(&packet_timestamp, packet->data + 1, sizeof(packet_timestamp));
      offset = sizeof(RakNetTime) + 1;
    }
    if (packet->length > offset) {
      switch (packet->data[offset]) {
        case ID_AUTH_KEY:
          if (ReadAuthenticationKey(*packet, offset, g_client->server_auth_key)) {
            samp::util::WriteLogLine(
                SendAuthenticationKeyResponse(*g_client, g_client->server_auth_key)
                    ? "network auth response sent"
                    : "network auth response generation or send failed");
          } else {
            samp::util::WriteLogLine("network auth key packet rejected");
          }
          break;
        case ID_CONNECTION_REQUEST_ACCEPTED:
          if (g_client->state != NetworkState::JoiningGame &&
              ReadConnectionAcceptance(*packet, offset, g_client->server_challenge)) {
            SendJoinRequest(*g_client);
          }
          break;
        case ID_CONNECTION_ATTEMPT_FAILED:
        case ID_NO_FREE_INCOMING_CONNECTIONS:
          g_client->state = NetworkState::Connecting;
          break;
        case ID_CONNECTION_LOST:
          peer.Disconnect(0);
          g_client->state = NetworkState::Connecting;
          break;
        case ID_DISCONNECTION_NOTIFICATION:
          peer.Disconnect(2000);
          break;
        case ID_CONNECTION_BANNED:
          samp::util::WriteLogLine("network connection rejected: banned");
          break;
        case ID_INVALID_PASSWORD:
          samp::util::WriteLogLine("network connection rejected: invalid password");
          peer.Disconnect(0);
          break;
        case ID_RSA_PUBLIC_KEY_MISMATCH:
          samp::util::WriteLogLine("network connection rejected: RSA key mismatch");
          break;
        case ID_PLAYER_SYNC:
          HandlePlayerSyncPacket(*packet, offset, packet_timestamp);
          break;
        case ID_VEHICLE_SYNC:
          HandleVehicleSyncPacket(*packet, offset, packet_timestamp);
          break;
        default:
          break;
      }
    }
    peer.DeallocatePacket(packet);
  }
}

void DestroyNetworkClient() {
  g_client.reset();
}

}
