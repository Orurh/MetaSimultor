#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
namespace engine {

struct TransformComponent {
  glm::vec3 position;
  glm::vec3 rotation = {0.0, 0.0, 0.0};
  glm::vec3 scale = {1.0, 1.0, 1.0};

  TransformComponent() = default;
  TransformComponent(const glm::vec3 &pos) : position(pos) {}
  TransformComponent(const TransformComponent &other) = default;

  glm::mat4 GetTransform() const {
    glm::vec3 rotationRad = glm::radians(rotation);
    auto rot = glm::toMat4(glm::quat(rotationRad));
    auto trans = glm::translate(glm::mat4(1.0f), position);
    auto scl = glm::scale(glm::mat4(1.0f), scale);
    return trans * rot * scl;
  }
};
}

struct Renderable {
    glm::vec3 color{1.0f};
};
