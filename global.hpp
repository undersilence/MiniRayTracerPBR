#pragma once
#include <iostream>
#include <cmath>
#include <random>
#include "RandomGen.hpp"
#undef M_PI
#define M_PI 3.141592653589793f

extern const float  EPSILON;
const float kInfinity = std::numeric_limits<float>::max();

inline float clamp(const float &lo, const float &hi, const float &v)
{ return std::max(lo, std::min(hi, v)); }

inline bool solveQuadratic(const float &a, const float &b, const float &c, float &x0, float &x1)
{
    float discr = b * b - 4 * a * c;
    if (discr < 0) return false;
    else if (discr == 0) x0 = x1 = - 0.5 * b / a;
    else {
        float q = (b > 0) ?
                  -0.5 * (b + sqrt(discr)) :
                  -0.5 * (b - sqrt(discr));
        x0 = q / a;
        x1 = c / q;
    }
    if (x0 > x1) std::swap(x0, x1);
    return true;
}

/*
Note:
on linux random_device would get one real random number
but on windows it would get one pseudo random number which cause the BUG
*/
thread_local static RandomGen<float> StaticRandomGen(23333, 0.f, 1.f);
inline float get_random_float(RandomGen<float> *random_gen = nullptr) {
    if(random_gen == nullptr) {
        return StaticRandomGen.get_random_float();
    }
    // distribution in range [1, 6]
    return random_gen->get_random_float();
}

inline void UpdateProgress(float progress)
{
    int barWidth = 70;

    std::cout << "[";
    int pos = barWidth * progress;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << " %\r";
    std::cout.flush();
};

inline Vector3f SphericalToCartesian(float theta, float phi) {
    float x = sinf(theta) * cosf(phi);
    float y = sinf(theta) * sinf(phi);
    float z = cosf(theta);
    return Vector3f(x, y, z);
}