#include "samp/game/length_decoder.h"

#include <cstddef>

namespace samp::game {
namespace {

struct OpcodeRow {
  unsigned flags;
  int handler;
};

const OpcodeRow g_primary_table[256] = {
#include "opcode_table.inc"
};

const OpcodeRow g_escape_table[256] = {
#include "opcode_table2.inc"
};

const unsigned char g_modrm_table[256] = {
#include "modrm_table.inc"
};

unsigned GenericLength(LengthDecoder &decoder, unsigned flags, const unsigned char *at) {
  unsigned base = 0;
  if ((flags >> 20) & 2) {
    base = decoder.wide_address ? (flags >> 11) & 7 : (flags >> 8) & 7;
  } else {
    base = decoder.wide_operand ? (flags >> 11) & 7 : (flags >> 8) & 7;
  }
  unsigned size = base;
  if ((flags >> 14) & 7) {
    unsigned index = (flags >> 14) & 7;
    unsigned char modrm = at[index];
    unsigned char entry = g_modrm_table[modrm];
    if ((entry & 0x10) && (at[index + 1] & 7) == 5) {
      unsigned char mode = modrm & 0xC0;
      if (mode != 0) {
        if (mode == 0x40) {
          size = base + 1;
        } else if (mode == 0x80) {
          size = base + 4;
        }
      } else {
        size = base + 4;
      }
    }
    size += entry & 0x0F;
  }
  return size;
}

unsigned DecodeWithRow(LengthDecoder &decoder, const OpcodeRow &row,
                       const OpcodeRow *table, const unsigned char *at);

unsigned DecodeTable(LengthDecoder &decoder, const OpcodeRow *table, const unsigned char *at) {
  return DecodeWithRow(decoder, table[at[0]], table, at);
}

unsigned DecodeWithRow(LengthDecoder &decoder, const OpcodeRow &row,
                       const OpcodeRow *table, const unsigned char *at) {
  switch (row.handler) {
    case 0:
      return GenericLength(decoder, row.flags, at);
    case 1:
      return DecodeTable(decoder, table, at + 1) + 1;
    case 2:
      return 1;
    case 3:
      return DecodeTable(decoder, g_escape_table, at + 1) + 1;
    case 4: {
      decoder.wide_operand = true;
      return DecodeTable(decoder, g_primary_table, at + 1) + 1;
    }
    case 5: {
      decoder.wide_address = true;
      return DecodeTable(decoder, g_primary_table, at + 1) + 1;
    }
    case 6: {
      unsigned flags = (at[1] & 0x38) != 0 ? 21238 : 23542;
      return GenericLength(decoder, flags, at);
    }
    case 7: {
      unsigned flags = (at[1] & 0x38) != 0 ? 21239 : 26359;
      return GenericLength(decoder, flags, at);
    }
    case 8:
      return GenericLength(decoder, 21247, at);
    default:
      return 0;
  }
}

}  // namespace

unsigned DecodeInstruction(LengthDecoder &decoder, const unsigned char *at) {
  if (!at) {
    return 0;
  }
  return DecodeTable(decoder, g_primary_table, at);
}

}  // namespace samp::game
