#ifndef LIGHT_TYPES
#define LIGHT_TYPES

#include <glm/glm.hpp>

namespace ic
{
enum class LightType : int
{
        Directional = 0,
        Point,
        Spot
};

struct alignas(16) GPULight
{
        glm::vec4 position;    // xyz = world pos,  w = LightType
        glm::vec4 direction;   // xyz = direction,  w = range
        glm::vec4 color;       // xyz = RGB,        w = intensity
        glm::vec4 spotAngles;  // x = cos(innerAngle), y = cos(outerAngle), zw = unused
};

inline constexpr int MAX_LIGHTS = 16;

// std140 layout for the whole light block
struct alignas(16) LightBlockData
{
        GPULight lights[MAX_LIGHTS];
        int      lightCount;
        float    _pad[3];
};

// std140 layout for per-frame camera data
struct alignas(16) PerFrameData
{
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec4 cameraPos;  // w unused
        float     time;
        float     _pad[3];
};

}  // namespace ic

#endif
