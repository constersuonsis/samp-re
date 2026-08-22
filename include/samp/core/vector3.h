#pragma once

namespace samp {

/// Three-component vector laid out exactly as the game engine stores it, so it
/// can be copied straight into and out of network payloads.
struct Vector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

static_assert(sizeof(Vector3) == 12, "Vector3 must stay a packed triple of floats");

/// Rotation quaternion in the order the client sends it: w first.
struct Quaternion {
    float w = 0.0f;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

static_assert(sizeof(Quaternion) == 16, "Quaternion must stay a packed quadruple of floats");

}  // namespace samp
