#pragma once
#include "entt.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace engine {
using EntityID = std::string;
struct IDComponent {
  EntityID ID;
  IDComponent(const std::string &id = "") : ID(id) {}
  std::string toString() const { return "IDComponent { ID: " + ID + " }"; }
  friend std::ostream &operator<<(std::ostream &os, const IDComponent &comp) {
    return os << comp.toString();
  }
};

struct TransformComponent {
  glm::vec3 position{0.0f};
  glm::vec3 scale{1.0f};
  glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};

  // ДОБАВЬ КОНСТРУКТОРЫ
  TransformComponent() = default;
  TransformComponent(const glm::vec3 &position) : position(position) {}
  TransformComponent(const glm::vec3 &position, const glm::vec3 &scale)
      : position(position), scale(scale) {}

  std::string toString() const {
    return "TransformComponent{Position: " + std::to_string(position.x) + ", " +
           std::to_string(position.y) + ", " + std::to_string(position.z) + "}";
  }
};

struct RenderComponent {
  std::string prefab_id;
  glm::vec3 scale{1, 1, 1};
  bool is_visible = true;

  RenderComponent(const std::string &id = "", const glm::vec3 &s = {1, 1, 1},
                  bool v = true)
      : prefab_id(id), scale(s), is_visible(v) {}
};

struct AgentLinkComponent {
  int gpu_index;
  std::string prefab_id;

  AgentLinkComponent(int idx = -1, const std::string &id = "")
      : gpu_index(idx), prefab_id(id) {}
};

struct FireEmitterComponent {
  float Intensity = 1.0f;

  FireEmitterComponent() = default;
  FireEmitterComponent(float intensity) : Intensity(intensity) {}
};
} // namespace engine
