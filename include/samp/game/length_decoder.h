#pragma once

namespace samp::game {

struct LengthDecoder {
  bool wide_operand = false;
  bool wide_address = false;
};

unsigned DecodeInstruction(LengthDecoder &decoder, const unsigned char *at);

}  // namespace samp::game
