#include "samp/client/security_archive.h"

#include "SHA1.h"

#define NOMINMAX
#include <windows.h>
#include <wincrypt.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <vector>

namespace samp::client {
namespace {

std::array<std::uint8_t, 20> g_game_hardware_state{};
bool g_game_hardware_state_captured = false;

std::uint32_t ReadLittleEndian32(const std::uint8_t *data) {
  return static_cast<std::uint32_t>(data[0]) |
         (static_cast<std::uint32_t>(data[1]) << 8) |
         (static_cast<std::uint32_t>(data[2]) << 16) |
         (static_cast<std::uint32_t>(data[3]) << 24);
}

void WriteLittleEndian32(std::uint8_t *data, std::uint32_t value) {
  data[0] = static_cast<std::uint8_t>(value);
  data[1] = static_cast<std::uint8_t>(value >> 8);
  data[2] = static_cast<std::uint8_t>(value >> 16);
  data[3] = static_cast<std::uint8_t>(value >> 24);
}

std::uint32_t TransformSecurityWord(std::uint32_t value) {
  return ((value ^ 0xC107FFFFu) >> 19) | ((value ^ 0x0003D3E7u) << 13);
}

bool ReadSecurityArchive(std::vector<std::uint8_t> &file_data) {
  char module_path[MAX_PATH] = {};
  const DWORD path_length = ::GetModuleFileNameA(nullptr, module_path, MAX_PATH);
  if (path_length == 0 || path_length >= MAX_PATH) {
    return false;
  }
  char *last_separator = std::strrchr(module_path, '\\');
  if (!last_separator) {
    last_separator = std::strrchr(module_path, '/');
  }
  if (!last_separator) {
    return false;
  }
  last_separator[1] = 0;
  std::string archive_path(module_path);
  archive_path += "samp.saa";
  std::ifstream archive(archive_path, std::ios::binary | std::ios::ate);
  if (!archive) {
    return false;
  }
  const std::streamoff archive_size = archive.tellg();
  if (archive_size < 256 + 2048 + 128 ||
      static_cast<std::uint64_t>(archive_size) > 64ull * 1024ull * 1024ull ||
      static_cast<std::uint64_t>(archive_size) > std::numeric_limits<std::size_t>::max()) {
    return false;
  }
  file_data.resize(static_cast<std::size_t>(archive_size));
  archive.seekg(0, std::ios::beg);
  archive.read(reinterpret_cast<char *>(file_data.data()), archive_size);
  return archive.good();
}

bool VerifySecurityArchive(const std::vector<std::uint8_t> &file_data) {
  static constexpr std::uint8_t encrypted_public_key[] = {
      0xAC, 0xA8, 0xAA, 0xAA, 0xAA, 0x8E, 0xAA, 0xAA, 0xF8, 0xF9, 0xEB, 0x9B,
      0xAA, 0xAE, 0xAA, 0xAA, 0xAB, 0xAA, 0xAB, 0xAA, 0x45, 0x7F, 0xED, 0xCD,
      0x03, 0x0A, 0x2A, 0x39, 0xF8, 0xC1, 0xCE, 0xB1, 0x9C, 0xC5, 0x86, 0x5A,
      0xA2, 0x1D, 0x0B, 0xBE, 0xE4, 0xF5, 0xF4, 0xA6, 0xE2, 0x44, 0x12, 0x06,
      0xEE, 0x4C, 0xC7, 0x4A, 0x76, 0xB9, 0xEC, 0xEC, 0x70, 0xC6, 0x22, 0x6A,
      0x3C, 0x9F, 0x30, 0x42, 0x15, 0xCB, 0x97, 0x88, 0xA9, 0x09, 0x1A, 0xA2,
      0xE2, 0x0A, 0x70, 0x5D, 0xDF, 0x71, 0x2E, 0x1E, 0xFF, 0x73, 0x13, 0xB1,
      0xE8, 0xE5, 0x93, 0x9F, 0x74, 0xF3, 0x4C, 0xE9, 0x98, 0xA4, 0x38, 0xD7,
      0x03, 0xC5, 0x88, 0x14, 0x8A, 0x59, 0x30, 0x55, 0xEB, 0x21, 0x4F, 0xDB,
      0xB5, 0xFE, 0x11, 0xEA, 0x7A, 0xBB, 0x2C, 0x94, 0x30, 0x9C, 0x7D, 0xD8,
      0x40, 0x29, 0x6D, 0x4E, 0x1E, 0x1D, 0x35, 0xBE, 0xA0, 0x5C, 0x33, 0x79,
      0xEC, 0xAC, 0x4C, 0x2D, 0x8C, 0x7C, 0x03, 0x02, 0xAA, 0x2F, 0x12, 0x33,
      0x03, 0x5E, 0xC6, 0x17};
  static_assert(sizeof(encrypted_public_key) == 148);
  const std::size_t signature_offset = file_data.size() - 128;
  if (signature_offset < 256 || signature_offset - 256 > std::numeric_limits<DWORD>::max()) {
    return false;
  }

  HCRYPTPROV provider = 0;
  HCRYPTKEY public_key = 0;
  HCRYPTHASH hash = 0;
  bool valid = false;
  if (!::CryptAcquireContextA(&provider, "SAMP", nullptr, PROV_RSA_FULL, 0) &&
      !::CryptAcquireContextA(&provider, "SAMP", nullptr, PROV_RSA_FULL, CRYPT_NEWKEYSET)) {
    return false;
  }
  std::array<std::uint8_t, sizeof(encrypted_public_key)> public_key_blob{};
  std::transform(std::begin(encrypted_public_key), std::end(encrypted_public_key),
                 public_key_blob.begin(), [](std::uint8_t value) { return value ^ 0xAA; });
  if (::CryptImportKey(provider, public_key_blob.data(),
                       static_cast<DWORD>(public_key_blob.size()), 0, 0, &public_key) &&
      ::CryptCreateHash(provider, CALG_SHA1, 0, 0, &hash) &&
      ::CryptHashData(hash, file_data.data() + 256,
                      static_cast<DWORD>(signature_offset - 256), 0)) {
    valid = ::CryptVerifySignatureA(hash, file_data.data() + signature_offset, 128,
                                   public_key, nullptr, 1) != FALSE;
  }
  if (hash) {
    ::CryptDestroyHash(hash);
  }
  if (public_key) {
    ::CryptDestroyKey(public_key);
  }
  ::CryptReleaseContext(provider, 0);
  return valid;
}

bool LoadHardwareIdResource(const std::vector<std::uint8_t> &file_data,
                            std::array<std::uint8_t, 100> &resource, bool &resource_found) {
  resource_found = false;
  const std::size_t signature_offset = file_data.size() - 128;
  const std::size_t index_offset = signature_offset - 2048;
  const std::uint32_t archive_magic = ReadLittleEndian32(file_data.data() + 248) ^
                                      ReadLittleEndian32(file_data.data() + 252);
  if ((archive_magic & 0xFFFFFu) != 0x83433u ||
      (archive_magic & 0x700000u) != 0x200000u) {
    return false;
  }

  std::array<std::uint8_t, 2048> index{};
  std::memcpy(index.data(), file_data.data() + index_offset, index.size());
  std::array<std::uint32_t, 4> index_key{};
  static constexpr std::uint8_t encrypted_index_key[16] = {
      0xB9, 0xEA, 0x40, 0x0A, 0xA3, 0x1F, 0x01, 0x23,
      0xB0, 0xEA, 0x46, 0x64, 0x78, 0xAF, 0x50, 0x80};
  CopySecurityBlock(index_key, encrypted_index_key, 0xAA);
  const SecurityBlockParameters parameters = InitializeSecurityBlockParameters(0x0CCF225Cu, 0x20u);
  if (!DecryptSecurityBlocks(index_key, parameters.initial_sum, parameters.delta,
                             parameters.rounds, index.data(), index.size())) {
    return false;
  }

  constexpr std::uint32_t hardware_id_hash = 0xBADDEA6Bu;
  const std::uint32_t archive_mask = 0xFFFFFFFFu;
  std::uint32_t descriptor = 0;
  std::size_t record_offset = index.size();
  for (std::size_t offset = 0; offset < index.size(); offset += 8) {
    if (ReadLittleEndian32(index.data() + offset) == hardware_id_hash) {
      record_offset = offset;
      descriptor = (ReadLittleEndian32(index.data() + offset) & archive_mask) ^
                   TransformSecurityWord(ReadLittleEndian32(index.data() + offset + 4));
      break;
    }
  }
  if (record_offset == index.size()) {
    return true;
  }
  resource_found = true;

  const std::uint32_t stored_length = descriptor >> 8;
  if (stored_length < resource.size() || stored_length > file_data.size()) {
    return false;
  }
  std::uint32_t storage_length = stored_length;
  const std::uint32_t resource_remainder = storage_length & 0x7FFu;
  if (resource_remainder != 0) {
    storage_length += 2048u - resource_remainder;
  }
  const std::uint8_t end_slot = static_cast<std::uint8_t>(ReadLittleEndian32(file_data.data() + 252) >> 5);
  std::uint8_t slot = static_cast<std::uint8_t>(descriptor);
  std::uint64_t data_offset = 256;
  for (std::size_t visited = 0; slot != end_slot && visited < 256; ++visited) {
    const std::size_t entry_offset = static_cast<std::size_t>(slot) * 8;
    const std::uint32_t entry_descriptor =
        (ReadLittleEndian32(index.data() + entry_offset) & archive_mask) ^
        TransformSecurityWord(ReadLittleEndian32(index.data() + entry_offset + 4));
    std::uint32_t entry_length = entry_descriptor >> 8;
    const std::uint32_t remainder = entry_length & 0x7FFu;
    if (remainder != 0) {
      entry_length += 2048u - remainder;
    }
    data_offset += entry_length;
    slot = static_cast<std::uint8_t>(entry_descriptor);
  }
  if (slot != end_slot || data_offset + storage_length > index_offset) {
    return false;
  }

  const std::size_t key_offset = hardware_id_hash % (index.size() - 16);
  std::array<std::uint32_t, 4> resource_key{};
  CopySecurityBlock(resource_key, index.data() + key_offset, 0);
  std::vector<std::uint8_t> encrypted_resource(storage_length);
  const std::size_t available_length = std::min<std::size_t>(encrypted_resource.size(),
                                                              index_offset - static_cast<std::size_t>(data_offset));
  if (available_length < encrypted_resource.size()) {
    return false;
  }
  std::memcpy(encrypted_resource.data(), file_data.data() + data_offset, encrypted_resource.size());
  if (!DecryptSecurityBlocks(resource_key, parameters.initial_sum, parameters.delta,
                             parameters.rounds, encrypted_resource.data(), encrypted_resource.size())) {
    return false;
  }
  std::copy_n(encrypted_resource.begin(), resource.size(), resource.begin());
  return true;
}

bool ReadGameHardwareState(std::array<std::uint8_t, 20> &state) {
  constexpr std::uintptr_t state_address = 0x4D3AA0u;
  MEMORY_BASIC_INFORMATION memory_info{};
  if (::VirtualQuery(reinterpret_cast<const void *>(state_address), &memory_info,
                     sizeof(memory_info)) != sizeof(memory_info) ||
      memory_info.State != MEM_COMMIT ||
      state_address + state.size() > reinterpret_cast<std::uintptr_t>(memory_info.BaseAddress) +
                                         memory_info.RegionSize) {
    return false;
  }
  std::memcpy(state.data(), reinterpret_cast<const void *>(state_address), state.size());
  return true;
}

}

SecurityBlockParameters InitializeSecurityBlockParameters(std::uint32_t seed,
                                                          std::uint32_t rounds) {
  const std::uint32_t delta = ((seed ^ 0xC107FFFFu) >> 19) |
                              ((seed ^ 0x0003D3E7u) << 13);
  return {delta * rounds, delta, rounds};
}

void CopySecurityBlock(std::array<std::uint32_t, 4> &destination,
                       const std::uint8_t *source, std::uint8_t xor_value) {
  auto *bytes = reinterpret_cast<std::uint8_t *>(destination.data());
  std::memcpy(bytes, source, sizeof(destination));
  if (xor_value != 0) {
    for (std::size_t index = 0; index < sizeof(destination); ++index) {
      bytes[index] ^= xor_value;
    }
  }
}

bool DecryptSecurityBlocks(std::array<std::uint32_t, 4> &state, std::uint32_t initial_sum,
                           std::uint32_t delta, std::uint32_t rounds, std::uint8_t *data,
                           std::size_t size) {
  if ((!data && size != 0) || (size % 8) != 0) {
    return false;
  }
  for (std::size_t offset = 0; offset < size; offset += 8) {
    std::uint32_t first = ReadLittleEndian32(data + offset);
    std::uint32_t second = ReadLittleEndian32(data + offset + 4);
    const std::uint32_t original_first = first;
    const std::uint32_t original_second = second;
    std::uint32_t sum = initial_sum;
    for (std::uint32_t iteration = 0; iteration < rounds; ++iteration) {
      second -= (sum + state[(sum >> 11) & 3]) ^
                (first + ((first << 4) ^ (first >> 5)));
      sum -= delta;
      first -= (sum + state[sum & 3]) ^ (second + ((second << 4) ^ (second >> 5)));
    }
    WriteLittleEndian32(data + offset, first);
    WriteLittleEndian32(data + offset + 4, second);
    state[0] ^= original_first;
    state[1] ^= original_second;
    state[2] ^= original_first;
    state[3] ^= original_second;
  }
  return true;
}

bool CaptureGameHardwareState() {
  g_game_hardware_state_captured = ReadGameHardwareState(g_game_hardware_state);
  return g_game_hardware_state_captured;
}

bool GenerateHardwareId(const char *server_auth_key, std::string &hardware_id) {
  if (!server_auth_key) {
    return false;
  }
  std::array<std::uint8_t, 100> security_resource{};
  std::vector<std::uint8_t> archive;
  if (!ReadSecurityArchive(archive) || !VerifySecurityArchive(archive)) {
    return false;
  }
  bool has_security_resource = false;
  if (!LoadHardwareIdResource(archive, security_resource, has_security_resource)) {
    return false;
  }

  CSHA1 sha1;
  std::string mutable_auth_key(server_auth_key);
  sha1.Update(reinterpret_cast<unsigned char *>(mutable_auth_key.data()),
              static_cast<unsigned int>(mutable_auth_key.size()));
  sha1.Final();
  const unsigned char *digest = sha1.GetHash();
  std::array<std::uint8_t, 20> hardware_state{};
  for (std::size_t index = 0; index < hardware_state.size(); ++index) {
    const std::size_t word_offset = index & ~static_cast<std::size_t>(3);
    hardware_state[index] = digest[word_offset + (3 - (index & 3))];
  }
  if (has_security_resource) {
    for (std::size_t index = 0; index < hardware_state.size(); ++index) {
      std::uint8_t xor_value = 0;
      if (index < 5) {
        xor_value = 0x2F;
      } else if (index < 10) {
        xor_value = 0x45;
      } else if (index < 15) {
        xor_value = 0x6F;
      } else {
        xor_value = 0xDB;
      }
      for (std::uint8_t resource_byte : security_resource) {
        hardware_state[index] ^= resource_byte ^ xor_value;
      }
    }
    if (!g_game_hardware_state_captured) {
      return false;
    }
    for (std::size_t index = 0; index < hardware_state.size(); ++index) {
      hardware_state[index] ^= g_game_hardware_state[index];
    }
  }

  static constexpr char hex_digits[] = "0123456789ABCDEF";
  hardware_id.resize(40);
  for (std::size_t index = 0; index < hardware_state.size(); ++index) {
    hardware_id[index * 2] = hex_digits[hardware_state[index] >> 4];
    hardware_id[index * 2 + 1] = hex_digits[hardware_state[index] & 0x0F];
  }
  return true;
}

}
