#include "samp/client/network_client.h"

#include "PacketEnumerations.h"
#include "RakClientInterface.h"
#include "RakNetworkFactory.h"
#include "SHA1.h"
#include "samp/client/security_archive.h"
#include "samp/client/spawn.h"
#include "samp/client/tick.h"
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
  std::uint8_t player_time_minute = 0;
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

struct PassengerSyncState {
  std::uint16_t vehicle_id = 0;
  std::uint8_t seat_flags = 0;
  std::uint8_t special_action = 0;
  std::uint8_t health = 0;
  std::uint8_t armor = 0;
  std::uint16_t left_right_analog = 0;
  std::uint16_t up_down_analog = 0;
  std::uint16_t keys = 0;
  std::array<float, 3> position{};
};

struct AimSyncState {
  std::uint8_t camera_mode = 0;
  std::array<float, 3> aim_direction{};
  std::array<float, 3> aim_position{};
  float drunk_level = 0.0f;
  std::uint8_t camera_weapon_state = 0;
  std::uint8_t aspect_ratio = 0;
};

struct BulletSyncState {
  std::uint8_t hit_type = 0;
  std::uint16_t hit_id = 0;
  std::array<float, 3> origin{};
  std::array<float, 3> target{};
  std::array<float, 3> center{};
  std::uint8_t weapon_id = 0;
};

struct TrailerSyncState {
  std::uint16_t trailer_id = 0;
  std::array<float, 3> matrix_position{};
  std::array<float, 4> rotation_quaternion{};
  std::array<float, 3> position{};
  std::array<float, 3> rotation{};
};

struct UnoccupiedSyncState {
  std::uint16_t vehicle_id = 0;
  std::uint8_t seat_id = 0;
  std::array<float, 3> roll{};
  std::array<float, 3> direction{};
  std::array<float, 3> position{};
  std::array<float, 3> velocity{};
  std::array<float, 3> turn_speed{};
  float vehicle_health = 0.0f;
};

struct MarkerSyncState {
  std::array<std::int16_t, 3> position{};
  std::uint32_t timestamp = 0;
  bool has_marker = false;
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
  PassengerSyncState passenger_sync_state{};
  AimSyncState aim_sync_state{};
  BulletSyncState bullet_sync_state{};
  TrailerSyncState trailer_sync_state{};
  UnoccupiedSyncState unoccupied_sync_state{};
  MarkerSyncState marker_sync_state{};
  RemotePlayerSpawnState spawn_state{};
  std::uint32_t last_sync_time = 0;
  std::uint32_t last_vehicle_sync_time = 0;
  std::uint32_t last_passenger_sync_time = 0;
  std::uint32_t last_aim_sync_time = 0;
  std::uint32_t last_bullet_sync_time = 0;
  std::uint32_t last_trailer_sync_time = 0;
  std::uint32_t last_unoccupied_sync_time = 0;
  std::uint32_t sync_packet_count = 0;
  std::uint32_t vehicle_sync_packet_count = 0;
  std::uint32_t passenger_sync_packet_count = 0;
  std::uint32_t aim_sync_packet_count = 0;
  std::uint32_t bullet_sync_packet_count = 0;
  std::uint32_t trailer_sync_packet_count = 0;
  std::uint32_t unoccupied_sync_packet_count = 0;
  std::uint32_t marker_sync_packet_count = 0;
  std::uint32_t color = 0;
  bool active = false;
  bool npc = false;
  bool has_sync_state = false;
  bool has_vehicle_sync_state = false;
  bool has_passenger_sync_state = false;
  bool has_aim_sync_state = false;
  bool has_bullet_sync_state = false;
  bool has_trailer_sync_state = false;
  bool has_unoccupied_sync_state = false;
  bool has_marker_sync_state = false;
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
  std::array<std::uint8_t, 46> spawn_info{};
  ServerSpawnInfo decoded_spawn_info;
  std::uint32_t selected_class_id = 0;
  std::uint8_t spawn_request_state = 0;
  std::array<RemotePlayer, 1005> remote_players{};
  bool has_server_game_settings = false;
  bool has_spawn_info = false;
  bool has_class_selection_response = false;
  bool class_selection_accepted = false;
  bool has_spawn_request_state = false;
  bool spawn_interpolation_pending = false;
  bool server_spawn_apply_pending = false;
  NetworkState state = NetworkState::Connecting;
};

std::unique_ptr<NetworkClient> g_client;
int g_init_game_rpc_id = 139;
int g_connection_rejected_rpc_id = 130;
int g_client_message_rpc_id = 93;
int g_server_join_rpc_id = 137;
int g_server_quit_rpc_id = 138;
int g_server_weather_rpc_id = 152;
int g_server_player_time_rpc_id = 29;
int g_server_world_time_rpc_id = 94;
int g_server_gravity_rpc_id = 146;
int g_server_spawn_info_rpc_id = 68;
int g_server_class_selection_rpc_id = 128;
int g_server_request_spawn_rpc_id = 129;
int g_client_spawn_rpc_id = 52;
int g_world_player_add_rpc_id = 32;
int g_world_player_death_rpc_id = 166;
int g_world_player_remove_rpc_id = 163;
constexpr std::size_t kStuntBonusFlagIndex = 4;
constexpr std::size_t kGravityValueIndex = 4;
constexpr std::size_t kWorldHourValueIndex = 0;
constexpr std::size_t kWeatherValueIndex = 1;

void ResetNetworkSession(NetworkClient &client) {
  for (RemotePlayer &player : client.remote_players) {
    player = {};
  }
  client.server_game_settings = {};
  client.spawn_info = {};
  client.decoded_spawn_info = {};
  client.selected_class_id = 0;
  client.has_spawn_info = false;
  client.has_class_selection_response = false;
  client.class_selection_accepted = false;
  client.spawn_request_state = 0;
  client.has_spawn_request_state = false;
  client.spawn_interpolation_pending = false;
  client.server_spawn_apply_pending = false;
  client.server_challenge = 0;
  client.server_auth_key.clear();
  client.has_server_game_settings = false;
  SyncTimeOfDay(12, 0);
  client.last_attempt = ::GetTickCount();
  client.state = NetworkState::Connecting;
}

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
  if (player_id >= 0x3EC || player_id >= g_client->remote_players.size()) {
    samp::util::WriteLogNumber("network server quit RPC rejected: invalid player id", player_id);
    return;
  }
  RemotePlayer &player = g_client->remote_players[player_id];
  if (!player.active) {
    return;
  }
  player = {};
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

void HandleServerGravityRpc(RPCParameters *parameters) {
  if (!parameters || !g_client || !parameters->input || parameters->numberOfBitsOfData < 32) {
    return;
  }
  RakNet::BitStream payload(parameters->input, (parameters->numberOfBitsOfData + 7) / 8, false);
  std::uint32_t gravity = 0;
  if (!payload.Read(gravity)) {
    samp::util::WriteLogLine("network gravity RPC rejected: invalid payload");
    return;
  }
  g_client->server_game_settings.values[kGravityValueIndex] = gravity;
  samp::game::WriteDword(reinterpret_cast<void *>(0x863984), gravity);
  samp::util::WriteLogNumber("network server gravity updated", gravity);
}

void HandleServerSpawnInfoRpc(RPCParameters *parameters) {
  constexpr int kSpawnInfoSize = 46;
  if (!parameters || !g_client || !parameters->input ||
      parameters->numberOfBitsOfData < kSpawnInfoSize * 8) {
    samp::util::WriteLogLine("network spawn info RPC rejected: truncated payload");
    return;
  }
  RakNet::BitStream payload(parameters->input, (parameters->numberOfBitsOfData + 7) / 8, false);
  std::array<std::uint8_t, kSpawnInfoSize> bytes{};
  if (!payload.Read(reinterpret_cast<char *>(bytes.data()), kSpawnInfoSize)) {
    samp::util::WriteLogLine("network spawn info RPC rejected: invalid payload");
    return;
  }
  ServerSpawnInfo spawn_info;
  spawn_info.team = bytes[0];
  std::memcpy(&spawn_info.model_id, bytes.data() + 1, sizeof(spawn_info.model_id));
  spawn_info.reserved = bytes[5];
  std::memcpy(spawn_info.position.data(), bytes.data() + 6, sizeof(spawn_info.position));
  std::memcpy(&spawn_info.rotation, bytes.data() + 18, sizeof(spawn_info.rotation));
  std::memcpy(spawn_info.weapons.data(), bytes.data() + 22, sizeof(spawn_info.weapons));
  std::memcpy(spawn_info.ammunition.data(), bytes.data() + 34,
              sizeof(spawn_info.ammunition));
  g_client->spawn_info = bytes;
  g_client->decoded_spawn_info = spawn_info;
  g_client->has_spawn_info = true;
  samp::util::WriteLogNumber("network server spawn model", spawn_info.model_id);
  samp::util::WriteLogNumber("network server spawn weapon 1", spawn_info.weapons[0]);
  samp::util::WriteLogNumber("network server spawn weapon 2", spawn_info.weapons[1]);
  samp::util::WriteLogNumber("network server spawn weapon 3", spawn_info.weapons[2]);
  samp::util::WriteLogLine("network server spawn info received");
}

void HandleServerClassSelectionRpc(RPCParameters *parameters) {
  constexpr int kClassSelectionResponseSize = 1 + 46;
  if (!parameters || !g_client || !parameters->input ||
      parameters->numberOfBitsOfData < kClassSelectionResponseSize * 8) {
    samp::util::WriteLogLine("network class selection RPC rejected: truncated payload");
    return;
  }
  RakNet::BitStream payload(parameters->input, (parameters->numberOfBitsOfData + 7) / 8, false);
  std::uint8_t accepted = 0;
  std::array<std::uint8_t, 46> bytes{};
  if (!payload.Read(accepted) ||
      !payload.Read(reinterpret_cast<char *>(bytes.data()), static_cast<int>(bytes.size()))) {
    samp::util::WriteLogLine("network class selection RPC rejected: invalid payload");
    return;
  }
  g_client->has_class_selection_response = true;
  g_client->class_selection_accepted = accepted != 0;
  if (g_client->class_selection_accepted) {
    ServerSpawnInfo spawn_info;
    spawn_info.team = bytes[0];
    std::memcpy(&spawn_info.model_id, bytes.data() + 1, sizeof(spawn_info.model_id));
    spawn_info.reserved = bytes[5];
    std::memcpy(spawn_info.position.data(), bytes.data() + 6,
                sizeof(spawn_info.position));
    std::memcpy(&spawn_info.rotation, bytes.data() + 18, sizeof(spawn_info.rotation));
    std::memcpy(spawn_info.weapons.data(), bytes.data() + 22, sizeof(spawn_info.weapons));
    std::memcpy(spawn_info.ammunition.data(), bytes.data() + 34,
                sizeof(spawn_info.ammunition));
    g_client->spawn_info = bytes;
    g_client->decoded_spawn_info = spawn_info;
    g_client->has_spawn_info = true;
  }
  samp::util::WriteLogNumber("network class selection accepted",
                             g_client->class_selection_accepted ? 1 : 0);
}

void HandleServerRequestSpawnRpc(RPCParameters *parameters) {
  if (!parameters || !g_client || !parameters->input || parameters->numberOfBitsOfData < 8) {
    samp::util::WriteLogLine("network spawn request RPC rejected: truncated payload");
    return;
  }
  RakNet::BitStream payload(parameters->input, (parameters->numberOfBitsOfData + 7) / 8, false);
  if (!payload.Read(g_client->spawn_request_state)) {
    samp::util::WriteLogLine("network spawn request RPC rejected: invalid payload");
    return;
  }
  g_client->has_spawn_request_state = true;
  samp::util::WriteLogNumber("network server spawn request state", g_client->spawn_request_state);
  const bool spawn_was_requested =
      g_client->spawn_request_state == 2 ||
      (g_client->spawn_request_state != 0 && g_client->spawn_interpolation_pending);
  if (!spawn_was_requested) {
    g_client->spawn_interpolation_pending = false;
    g_client->server_spawn_apply_pending = false;
    samp::util::WriteLogLine("network server spawn request ignored");
    return;
  }
  g_client->server_spawn_apply_pending = true;
  if (!g_client->has_spawn_info) {
    samp::util::WriteLogLine("network server spawn request waiting for spawn info");
    return;
  }
  samp::util::WriteLogLine("network server spawn request accepted");
}

void ProcessPendingServerSpawn() {
  if (!g_client || !g_client->server_spawn_apply_pending || !g_client->has_spawn_info) {
    return;
  }
  if (!ApplyServerSpawnInfo(g_client->decoded_spawn_info)) {
    g_client->server_spawn_apply_pending = false;
    g_client->spawn_interpolation_pending = false;
    samp::util::WriteLogLine("network server spawn application failed");
    return;
  }
  RakNet::BitStream payload;
  const bool response_sent = g_client->peer->RPC(
      &g_client_spawn_rpc_id, &payload, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false,
      UNASSIGNED_NETWORK_ID, nullptr);
  g_client->server_spawn_apply_pending = false;
  g_client->spawn_interpolation_pending = false;
  samp::util::WriteLogLine(response_sent ? "network spawn response sent"
                                         : "network spawn response send failed");
}

void HandleServerWorldTimeRpc(RPCParameters *parameters) {
  if (!parameters || !g_client || !parameters->input || parameters->numberOfBitsOfData < 8) {
    return;
  }
  RakNet::BitStream payload(parameters->input, (parameters->numberOfBitsOfData + 7) / 8, false);
  std::uint8_t hour = 0;
  if (!payload.Read(hour)) {
    samp::util::WriteLogLine("network world time RPC rejected: invalid payload");
    return;
  }
  g_client->server_game_settings.byte_values[kWorldHourValueIndex] = hour;
  samp::util::WriteLogNumber("network world hour updated", hour);
}

void HandleServerPlayerTimeRpc(RPCParameters *parameters) {
  if (!parameters || !g_client || !parameters->input || parameters->numberOfBitsOfData < 16) {
    return;
  }
  RakNet::BitStream payload(parameters->input, (parameters->numberOfBitsOfData + 7) / 8, false);
  std::uint8_t hour = 0;
  std::uint8_t minute = 0;
  if (!payload.Read(hour) || !payload.Read(minute)) {
    samp::util::WriteLogLine("network player time RPC rejected: invalid payload");
    return;
  }
  g_client->server_game_settings.byte_values[kWorldHourValueIndex] = hour;
  g_client->server_game_settings.player_time_minute = minute;
  SyncTimeOfDay(hour, minute);
  samp::util::WriteLogNumber("network player hour updated", hour);
  samp::util::WriteLogNumber("network player minute updated", minute);
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
  player.has_passenger_sync_state = false;
  player.has_aim_sync_state = false;
  player.has_bullet_sync_state = false;
  player.has_trailer_sync_state = false;
  player.has_unoccupied_sync_state = false;
  player.has_marker_sync_state = false;
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
  player.sync_state = {};
  player.vehicle_sync_state = {};
  player.passenger_sync_state = {};
  player.aim_sync_state = {};
  player.bullet_sync_state = {};
  player.trailer_sync_state = {};
  player.unoccupied_sync_state = {};
  player.marker_sync_state = {};
  player.spawn_state = {};
  player.last_sync_time = 0;
  player.last_vehicle_sync_time = 0;
  player.last_passenger_sync_time = 0;
  player.last_aim_sync_time = 0;
  player.last_bullet_sync_time = 0;
  player.last_trailer_sync_time = 0;
  player.last_unoccupied_sync_time = 0;
  player.world_spawned = false;
  player.has_spawn_state = false;
  player.has_sync_state = false;
  player.has_vehicle_sync_state = false;
  player.has_passenger_sync_state = false;
  player.has_aim_sync_state = false;
  player.has_bullet_sync_state = false;
  player.has_trailer_sync_state = false;
  player.has_unoccupied_sync_state = false;
  player.has_marker_sync_state = false;
  samp::util::WriteLogNumber("network remote player despawn received", player_id);
}

void HandleWorldPlayerDeathRpc(RPCParameters *parameters) {
  if (!parameters || !g_client || !parameters->input || parameters->numberOfBitsOfData < 16) {
    samp::util::WriteLogLine("network world player death RPC rejected: truncated payload");
    return;
  }
  RakNet::BitStream payload(parameters->input, (parameters->numberOfBitsOfData + 7) / 8, false);
  std::uint16_t player_id = 0;
  if (!payload.Read(player_id) || player_id > 0x3EC ||
      player_id >= g_client->remote_players.size()) {
    samp::util::WriteLogLine("network world player death RPC rejected: invalid player id");
    return;
  }
  RemotePlayer &player = g_client->remote_players[player_id];
  if (!player.active) {
    return;
  }
  player.vehicle_sync_state = {};
  player.passenger_sync_state = {};
  player.last_vehicle_sync_time = 0;
  player.last_passenger_sync_time = 0;
  player.has_vehicle_sync_state = false;
  player.has_passenger_sync_state = false;
  samp::util::WriteLogNumber("network remote player left vehicle", player_id);
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

bool ReadPassengerSyncState(RakNet::BitStream &payload, PassengerSyncState &state) {
  std::array<std::uint8_t, 24> bytes{};
  if (!payload.Read(reinterpret_cast<char *>(bytes.data()), static_cast<int>(bytes.size()))) {
    return false;
  }
  PassengerSyncState parsed;
  std::memcpy(&parsed.vehicle_id, bytes.data(), sizeof(parsed.vehicle_id));
  parsed.seat_flags = bytes[2];
  parsed.special_action = bytes[3] & 0x3F;
  parsed.health = bytes[4];
  parsed.armor = bytes[5];
  std::memcpy(&parsed.left_right_analog, bytes.data() + 6, sizeof(parsed.left_right_analog));
  std::memcpy(&parsed.up_down_analog, bytes.data() + 8, sizeof(parsed.up_down_analog));
  std::memcpy(&parsed.keys, bytes.data() + 10, sizeof(parsed.keys));
  std::memcpy(parsed.position.data(), bytes.data() + 12, sizeof(parsed.position));
  state = parsed;
  return true;
}

bool ReadAimSyncState(RakNet::BitStream &payload, AimSyncState &state) {
  std::array<std::uint8_t, 31> bytes{};
  if (!payload.Read(reinterpret_cast<char *>(bytes.data()), static_cast<int>(bytes.size()))) {
    return false;
  }
  AimSyncState parsed;
  parsed.camera_mode = bytes[0];
  std::memcpy(parsed.aim_direction.data(), bytes.data() + 1, sizeof(parsed.aim_direction));
  std::memcpy(parsed.aim_position.data(), bytes.data() + 13, sizeof(parsed.aim_position));
  std::memcpy(&parsed.drunk_level, bytes.data() + 25, sizeof(parsed.drunk_level));
  parsed.camera_weapon_state = bytes[29];
  parsed.aspect_ratio = bytes[30];
  state = parsed;
  return true;
}

bool ReadBulletSyncState(RakNet::BitStream &payload, BulletSyncState &state) {
  std::array<std::uint8_t, 40> bytes{};
  if (!payload.Read(reinterpret_cast<char *>(bytes.data()), static_cast<int>(bytes.size()))) {
    return false;
  }
  BulletSyncState parsed;
  parsed.hit_type = bytes[0];
  std::memcpy(&parsed.hit_id, bytes.data() + 1, sizeof(parsed.hit_id));
  std::memcpy(parsed.origin.data(), bytes.data() + 3, sizeof(parsed.origin));
  std::memcpy(parsed.target.data(), bytes.data() + 15, sizeof(parsed.target));
  std::memcpy(parsed.center.data(), bytes.data() + 27, sizeof(parsed.center));
  parsed.weapon_id = bytes[39];
  state = parsed;
  return true;
}

bool ReadTrailerSyncState(RakNet::BitStream &payload, TrailerSyncState &state) {
  std::array<std::uint8_t, 54> bytes{};
  if (!payload.Read(reinterpret_cast<char *>(bytes.data()), static_cast<int>(bytes.size()))) {
    return false;
  }
  TrailerSyncState parsed;
  std::memcpy(&parsed.trailer_id, bytes.data(), sizeof(parsed.trailer_id));
  std::memcpy(parsed.matrix_position.data(), bytes.data() + 2,
              sizeof(parsed.matrix_position));
  std::memcpy(parsed.rotation_quaternion.data(), bytes.data() + 14,
              sizeof(parsed.rotation_quaternion));
  std::memcpy(parsed.position.data(), bytes.data() + 30, sizeof(parsed.position));
  std::memcpy(parsed.rotation.data(), bytes.data() + 42, sizeof(parsed.rotation));
  state = parsed;
  return true;
}

bool IsUnitVector(const std::array<float, 3> &vector) {
  for (float component : vector) {
    if (!(component >= -1.0f && component <= 1.0f)) {
      return false;
    }
  }
  return true;
}

bool IsValidSyncPosition(const std::array<float, 3> &position) {
  return position[0] > -20000.0f && position[0] < 20000.0f &&
         position[1] > -20000.0f && position[1] < 20000.0f &&
         position[2] > -10000.0f && position[2] < 100000.0f;
}

bool IsSmallSyncVector(const std::array<float, 3> &vector) {
  for (float component : vector) {
    if (!(component > -100.0f && component < 100.0f)) {
      return false;
    }
  }
  return true;
}

bool ReadUnoccupiedSyncState(RakNet::BitStream &payload, UnoccupiedSyncState &state) {
  std::array<std::uint8_t, 67> bytes{};
  if (!payload.Read(reinterpret_cast<char *>(bytes.data()), static_cast<int>(bytes.size()))) {
    return false;
  }
  UnoccupiedSyncState parsed;
  std::memcpy(&parsed.vehicle_id, bytes.data(), sizeof(parsed.vehicle_id));
  parsed.seat_id = bytes[2];
  std::memcpy(parsed.roll.data(), bytes.data() + 3, sizeof(parsed.roll));
  std::memcpy(parsed.direction.data(), bytes.data() + 15, sizeof(parsed.direction));
  std::memcpy(parsed.position.data(), bytes.data() + 27, sizeof(parsed.position));
  std::memcpy(parsed.velocity.data(), bytes.data() + 39, sizeof(parsed.velocity));
  std::memcpy(parsed.turn_speed.data(), bytes.data() + 51, sizeof(parsed.turn_speed));
  std::memcpy(&parsed.vehicle_health, bytes.data() + 63, sizeof(parsed.vehicle_health));
  if (parsed.vehicle_id == 0 || parsed.vehicle_id == 0xFFFF || parsed.vehicle_id >= 2000 ||
      !IsUnitVector(parsed.roll) || !IsUnitVector(parsed.direction) ||
      !IsValidSyncPosition(parsed.position) || !IsSmallSyncVector(parsed.velocity) ||
      !IsSmallSyncVector(parsed.turn_speed)) {
    return false;
  }
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

void HandlePassengerSyncPacket(Packet &packet, unsigned offset, std::uint32_t packet_timestamp) {
  constexpr unsigned kPassengerSyncPacketSize = 1 + sizeof(std::uint16_t) + 24;
  if (!g_client || g_client->state != NetworkState::Active ||
      packet.length - offset < kPassengerSyncPacketSize) {
    return;
  }
  RakNet::BitStream payload(packet.data + offset, packet.length - offset, false);
  unsigned char packet_id = 0;
  std::uint16_t player_id = 0;
  if (!payload.Read(packet_id) || packet_id != ID_PASSENGER_SYNC || !payload.Read(player_id) ||
      player_id >= g_client->remote_players.size()) {
    return;
  }
  RemotePlayer &player = g_client->remote_players[player_id];
  if (!player.active) {
    return;
  }
  PassengerSyncState sync_state;
  if (!ReadPassengerSyncState(payload, sync_state)) {
    samp::util::WriteLogNumber("network passenger sync packet rejected: malformed", player_id);
    return;
  }
  if (packet_timestamp != 0 && player.has_passenger_sync_state &&
      static_cast<std::int32_t>(packet_timestamp - player.last_passenger_sync_time) < 0) {
    return;
  }
  player.passenger_sync_state = sync_state;
  player.last_passenger_sync_time = packet_timestamp;
  player.has_passenger_sync_state = true;
  ++player.passenger_sync_packet_count;
}

void HandleAimSyncPacket(Packet &packet, unsigned offset, std::uint32_t packet_timestamp) {
  constexpr unsigned kAimSyncPacketSize = 1 + sizeof(std::uint16_t) + 31;
  if (!g_client || g_client->state != NetworkState::Active ||
      packet.length - offset < kAimSyncPacketSize) {
    return;
  }
  RakNet::BitStream payload(packet.data + offset, packet.length - offset, false);
  unsigned char packet_id = 0;
  std::uint16_t player_id = 0;
  if (!payload.Read(packet_id) || packet_id != ID_AIM_SYNC || !payload.Read(player_id) ||
      player_id >= g_client->remote_players.size()) {
    return;
  }
  RemotePlayer &player = g_client->remote_players[player_id];
  if (!player.active) {
    return;
  }
  AimSyncState sync_state;
  if (!ReadAimSyncState(payload, sync_state)) {
    samp::util::WriteLogNumber("network aim sync packet rejected: malformed", player_id);
    return;
  }
  if (packet_timestamp != 0 && player.has_aim_sync_state &&
      static_cast<std::int32_t>(packet_timestamp - player.last_aim_sync_time) < 0) {
    return;
  }
  player.aim_sync_state = sync_state;
  player.last_aim_sync_time = packet_timestamp;
  player.has_aim_sync_state = true;
  ++player.aim_sync_packet_count;
}

void HandleBulletSyncPacket(Packet &packet, unsigned offset, std::uint32_t packet_timestamp) {
  constexpr unsigned kBulletSyncPacketSize = 1 + sizeof(std::uint16_t) + 40;
  if (!g_client || g_client->state != NetworkState::Active ||
      packet.length - offset < kBulletSyncPacketSize) {
    return;
  }
  RakNet::BitStream payload(packet.data + offset, packet.length - offset, false);
  unsigned char packet_id = 0;
  std::uint16_t player_id = 0;
  if (!payload.Read(packet_id) || packet_id != ID_BULLET_SYNC || !payload.Read(player_id) ||
      player_id >= 0x3EC || player_id >= g_client->remote_players.size()) {
    return;
  }
  RemotePlayer &player = g_client->remote_players[player_id];
  if (!player.active || !player.world_spawned) {
    return;
  }
  BulletSyncState sync_state;
  if (!ReadBulletSyncState(payload, sync_state)) {
    samp::util::WriteLogNumber("network bullet sync packet rejected: malformed", player_id);
    return;
  }
  if (packet_timestamp != 0 && player.has_bullet_sync_state &&
      static_cast<std::int32_t>(packet_timestamp - player.last_bullet_sync_time) < 0) {
    return;
  }
  player.bullet_sync_state = sync_state;
  player.last_bullet_sync_time = packet_timestamp;
  player.has_bullet_sync_state = true;
  ++player.bullet_sync_packet_count;
}

void HandleTrailerSyncPacket(Packet &packet, unsigned offset, std::uint32_t packet_timestamp) {
  constexpr unsigned kTrailerSyncPacketSize = 1 + sizeof(std::uint16_t) + 54;
  if (!g_client || g_client->state != NetworkState::Active ||
      packet.length - offset < kTrailerSyncPacketSize) {
    return;
  }
  RakNet::BitStream payload(packet.data + offset, packet.length - offset, false);
  unsigned char packet_id = 0;
  std::uint16_t player_id = 0;
  if (!payload.Read(packet_id) || packet_id != ID_TRAILER_SYNC || !payload.Read(player_id) ||
      player_id > 0x3EC || player_id >= g_client->remote_players.size()) {
    return;
  }
  RemotePlayer &player = g_client->remote_players[player_id];
  if (!player.active) {
    return;
  }
  TrailerSyncState sync_state;
  if (!ReadTrailerSyncState(payload, sync_state)) {
    samp::util::WriteLogNumber("network trailer sync packet rejected: malformed", player_id);
    return;
  }
  if (packet_timestamp != 0 && player.has_trailer_sync_state &&
      static_cast<std::int32_t>(packet_timestamp - player.last_trailer_sync_time) < 0) {
    return;
  }
  player.trailer_sync_state = sync_state;
  player.last_trailer_sync_time = packet_timestamp;
  player.has_trailer_sync_state = true;
  ++player.trailer_sync_packet_count;
}

void HandleUnoccupiedSyncPacket(Packet &packet, unsigned offset,
                                std::uint32_t packet_timestamp) {
  constexpr unsigned kUnoccupiedSyncPacketSize = 1 + sizeof(std::uint16_t) + 67;
  if (!g_client || g_client->state != NetworkState::Active ||
      packet.length - offset < kUnoccupiedSyncPacketSize) {
    return;
  }
  RakNet::BitStream payload(packet.data + offset, packet.length - offset, false);
  unsigned char packet_id = 0;
  std::uint16_t player_id = 0;
  if (!payload.Read(packet_id) || packet_id != ID_UNOCCUPIED_SYNC || !payload.Read(player_id) ||
      player_id > 0x3EC || player_id >= g_client->remote_players.size()) {
    return;
  }
  RemotePlayer &player = g_client->remote_players[player_id];
  if (!player.active) {
    return;
  }
  UnoccupiedSyncState sync_state;
  if (!ReadUnoccupiedSyncState(payload, sync_state)) {
    samp::util::WriteLogNumber("network unoccupied sync packet rejected: malformed", player_id);
    return;
  }
  if (packet_timestamp != 0 && player.has_unoccupied_sync_state &&
      static_cast<std::int32_t>(packet_timestamp - player.last_unoccupied_sync_time) < 0) {
    return;
  }
  player.unoccupied_sync_state = sync_state;
  player.last_unoccupied_sync_time = packet_timestamp;
  player.has_unoccupied_sync_state = true;
  ++player.unoccupied_sync_packet_count;
}

void HandleMarkersSyncPacket(Packet &packet, unsigned offset,
                             std::uint32_t packet_timestamp) {
  if (!g_client || g_client->state != NetworkState::Active || packet.length - offset < 5) {
    return;
  }
  RakNet::BitStream payload(packet.data + offset, packet.length - offset, false);
  unsigned char packet_id = 0;
  std::uint32_t marker_count = 0;
  if (!payload.Read(packet_id) || packet_id != ID_MARKERS_SYNC || !payload.Read(marker_count) ||
      marker_count > g_client->remote_players.size()) {
    samp::util::WriteLogLine("network markers sync packet rejected: invalid header");
    return;
  }
  bool previous_marker_state = true;
  for (std::uint32_t index = 0; index < marker_count; ++index) {
    std::uint16_t player_id = 0;
    bool has_marker = previous_marker_state;
    MarkerSyncState marker_state;
    if (!payload.Read(player_id) ||
        (payload.GetNumberOfUnreadBits() > 0 && !payload.Read(has_marker))) {
      samp::util::WriteLogLine("network markers sync packet rejected: truncated entry");
      return;
    }
    previous_marker_state = has_marker;
    if (has_marker &&
        (!payload.Read(marker_state.position[0]) || !payload.Read(marker_state.position[1]) ||
         !payload.Read(marker_state.position[2]))) {
      samp::util::WriteLogLine("network markers sync packet rejected: truncated position");
      return;
    }
    if (player_id >= 0x3EC || player_id >= g_client->remote_players.size()) {
      continue;
    }
    RemotePlayer &player = g_client->remote_players[player_id];
    if (!player.active) {
      continue;
    }
    marker_state.has_marker = has_marker;
    marker_state.timestamp = packet_timestamp;
    player.marker_sync_state = marker_state;
    player.has_marker_sync_state = true;
    ++player.marker_sync_packet_count;
  }
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
  client->peer->RegisterAsRemoteProcedureCall(&g_server_gravity_rpc_id, &HandleServerGravityRpc);
  client->peer->RegisterAsRemoteProcedureCall(&g_server_spawn_info_rpc_id,
                                               &HandleServerSpawnInfoRpc);
  client->peer->RegisterAsRemoteProcedureCall(&g_server_class_selection_rpc_id,
                                               &HandleServerClassSelectionRpc);
  client->peer->RegisterAsRemoteProcedureCall(&g_server_request_spawn_rpc_id,
                                               &HandleServerRequestSpawnRpc);
  client->peer->RegisterAsRemoteProcedureCall(&g_server_player_time_rpc_id,
                                               &HandleServerPlayerTimeRpc);
  client->peer->RegisterAsRemoteProcedureCall(&g_server_world_time_rpc_id,
                                               &HandleServerWorldTimeRpc);
  client->peer->RegisterAsRemoteProcedureCall(&g_world_player_add_rpc_id,
                                               &HandleWorldPlayerAddRpc);
  client->peer->RegisterAsRemoteProcedureCall(&g_world_player_death_rpc_id,
                                               &HandleWorldPlayerDeathRpc);
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

bool RequestServerClassSelection(std::uint32_t class_id) {
  if (!g_client || g_client->state != NetworkState::Active) {
    return false;
  }
  RakNet::BitStream payload;
  payload.Write(class_id);
  if (!g_client->peer->RPC(&g_server_class_selection_rpc_id, &payload, HIGH_PRIORITY, RELIABLE,
                           0, false, UNASSIGNED_NETWORK_ID, nullptr)) {
    samp::util::WriteLogLine("network class selection send failed");
    return false;
  }
  g_client->selected_class_id = class_id;
  g_client->has_class_selection_response = false;
  g_client->class_selection_accepted = false;
  samp::util::WriteLogNumber("network class selection sent", class_id);
  return true;
}

bool RequestServerSpawn() {
  if (!g_client || g_client->state != NetworkState::Active ||
      g_client->spawn_interpolation_pending || !g_client->class_selection_accepted) {
    return false;
  }
  RakNet::BitStream payload;
  if (!g_client->peer->RPC(&g_server_request_spawn_rpc_id, &payload, HIGH_PRIORITY, RELIABLE, 0,
                           false, UNASSIGNED_NETWORK_ID, nullptr)) {
    samp::util::WriteLogLine("network spawn request send failed");
    return false;
  }
  g_client->spawn_interpolation_pending = true;
  samp::util::WriteLogLine("network spawn request sent");
  return true;
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
          g_client->state = NetworkState::Connecting;
          samp::util::WriteLogLine("network connection attempt failed; retrying");
          break;
        case ID_NO_FREE_INCOMING_CONNECTIONS:
          g_client->state = NetworkState::Connecting;
          samp::util::WriteLogLine("network server is full; retrying");
          break;
        case ID_CONNECTION_LOST:
          peer.Disconnect(0);
          ResetNetworkSession(*g_client);
          samp::util::WriteLogLine("network connection lost; session cleared, reconnecting");
          break;
        case ID_DISCONNECTION_NOTIFICATION:
          peer.Disconnect(2000);
          ResetNetworkSession(*g_client);
          samp::util::WriteLogLine("network server closed the connection; session cleared, reconnecting");
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
        case ID_PASSENGER_SYNC:
          HandlePassengerSyncPacket(*packet, offset, packet_timestamp);
          break;
        case ID_AIM_SYNC:
          HandleAimSyncPacket(*packet, offset, packet_timestamp);
          break;
        case ID_BULLET_SYNC:
          HandleBulletSyncPacket(*packet, offset, packet_timestamp);
          break;
        case ID_TRAILER_SYNC:
          HandleTrailerSyncPacket(*packet, offset, packet_timestamp);
          break;
        case ID_UNOCCUPIED_SYNC:
          HandleUnoccupiedSyncPacket(*packet, offset, packet_timestamp);
          break;
        case ID_MARKERS_SYNC:
          HandleMarkersSyncPacket(*packet, offset, packet_timestamp);
          break;
        default:
          break;
      }
    }
    peer.DeallocatePacket(packet);
  }
  ProcessPendingServerSpawn();
}

void DestroyNetworkClient() {
  g_client.reset();
}

}
