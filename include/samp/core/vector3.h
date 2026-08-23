#pragma once

namespace samp {

struct Vector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

static_assert(sizeof(Vector3) == 12, "Vector3 must stay a packed triple of floats");

struct Quaternion {
    float w = 0.0f;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

static_assert(sizeof(Quaternion) == 16, "Quaternion must stay a packed quadruple of floats");

}
