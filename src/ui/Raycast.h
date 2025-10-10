#pragma once
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>

namespace math {
inline void ScreenPointToRay(float px, float py, float viewportW, float viewportH,
                             const glm::mat4& proj, const glm::mat4& view,
                             glm::vec3& outOrigin, glm::vec3& outDir) {
    float x = (2.0F * px / viewportW) - 1.0F;
    float y = 1.0F - (2.0F * py / viewportH); // flip Y
    glm::vec4 ray_clip(x, y, -1.0F, 1.0F);

    glm::mat4 inv_proj = glm::inverse(proj);
    glm::vec4 ray_eye = inv_proj * ray_clip; // camera space
    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f);

    glm::mat4 inv_view = glm::inverse(view);
    glm::vec4 ray_world4 = inv_view * ray_eye;
    glm::vec3 ray_world = glm::normalize(glm::vec3(ray_world4));

    auto cam_pos = glm::vec3(inv_view[3]);
    outOrigin = cam_pos;
    outDir = ray_world;
}


inline bool IntersectRayPlaneY0(const glm::vec3& ro, const glm::vec3& rd, glm::vec3& hit) {
    if (std::fabs(rd.y) < 1e-6f) return false;
    float t = -ro.y / rd.y;
    if (t < 0.0f) return false;
    hit = ro + rd * t;
    return true;
}

}  // namespace math