#include "Scene.hpp"

#pragma once
struct hit_payload {
    float tNear;
    uint32_t index;
    Vector2f uv;
    Object* hit_obj;
};

struct MonotaskInfo {
    int lowIndex, highIndex;
    Vector3f eye_pos;
    int spp;
    std::vector<Vector3f>& bufferRef;

    MonotaskInfo() = default;
    MonotaskInfo(int low_index, int high_index, const Vector3f& eye_pos, int spp, std::vector<Vector3f>& buffer_ref)
        : lowIndex(low_index),
        highIndex(high_index),
        eye_pos(eye_pos),
        spp(spp),
        bufferRef(buffer_ref) {}

};

class Renderer {
public:
    int spp = 16;
    int num_of_thread = 12;
    void RenderMonotask(MonotaskInfo info, const Scene& scene, bool displayProgress = false);
    void Render(const Scene& scene);
    void RenderMultithread(const Scene& scene);
    void SavePPM(const char* filename, int width, int height, std::vector<Vector3f>& framebuffer) const;
private:
};


