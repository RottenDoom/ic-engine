#ifndef MATH_H
#define MATH_H

/**TODO: Improve this so that users dont have to use it or something. */

#define GLM_FORCE_RADIANS
#define GLM_FORCE_XYZW_ONLY
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <stdint.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

using vec2    = glm::vec2;
using vec3    = glm::vec3;
using vec4    = glm::vec4;

using dvec2   = glm::dvec2;
using dvec3   = glm::dvec3;
using dvec4   = glm::dvec4;

using i8vec2  = glm::vec<2, int8_t>;
using i8vec3  = glm::vec<3, int8_t>;
using i8vec4  = glm::vec<4, int8_t>;

using i16vec2 = glm::vec<2, int16_t>;
using i16vec3 = glm::vec<3, int16_t>;
using i16vec4 = glm::vec<4, int16_t>;

using ivec2   = glm::vec<2, int32_t>;
using ivec3   = glm::vec<3, int32_t>;
using ivec4   = glm::vec<4, int32_t>;

using u8vec2  = glm::vec<2, uint8_t>;
using u8vec3  = glm::vec<3, uint8_t>;
using u8vec4  = glm::vec<4, uint8_t>;

using u16vec2 = glm::vec<2, uint16_t>;
using u16vec3 = glm::vec<3, uint16_t>;
using u16vec4 = glm::vec<4, uint16_t>;

using uvec2   = glm::vec<2, uint32_t>;
using uvec3   = glm::vec<3, uint32_t>;
using uvec4   = glm::vec<4, uint32_t>;

#endif