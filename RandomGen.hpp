#include <random>
template<class T = float>
struct RandomGen {
    std::mt19937 rng;
    std::uniform_real_distribution<T> dist;
    RandomGen(int seed, T low, T high):rng(seed), dist(low, high) {}
    float get_random_float() {
        return dist(rng);
    }
};