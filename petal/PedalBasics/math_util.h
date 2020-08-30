#pragma once

#define ONE_THIRD 0.3333333333333333333333f
#define TWO_THIRDS 0.6666666666666666666666f

inline float avg(float a, float b) {
  return a + b / 2.0f;
}

inline float max(float a, float b) {
    return a >= b ? a : b;
}

inline float min(float a, float b) {
    return a >= b ? b : a;
}

inline int max(int a, int b) {
    return a >= b ? a : b;
}

inline int min(int a, int b) {
    return a >= b ? b : a;
}

inline int clamp(int value, int minimum, int maximum) {
    return max(minimum, min(maximum, value));
}

inline int mod(int a, int b) { return (a%b+b)%b; }

inline float sign(float a) {
    return a >= 0 ? 1.0f : -1.0f;
}

