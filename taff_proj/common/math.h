#pragma once

namespace common {

inline float wrap_180(float deg) {
    while (deg >  180.0f) deg -= 360.0f;
    while (deg < -180.0f) deg += 360.0f;
    return deg;
}

inline float wrap_360(float deg) {
    while (deg >= 360.0f) deg -= 360.0f;
    while (deg <  0.0f)  deg += 360.0f;
    return deg;
}

inline float clamp(float x, float lo, float hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}

}