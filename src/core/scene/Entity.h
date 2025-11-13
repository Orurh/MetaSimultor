#pragma once
#include "Components.h"
#include "Scene.h"
#include "entt.hpp"
namespace engine {
template <typename T>
concept HasToString = requires(T t) {
  { t.toString() } -> std::convertible_to<std::string>;
};
class Entity {
public:
  Entity() = default;
  Entity(const Entity &other) = default;
  Entity(entt::entity handle, Scene *scene);

  template <typename T, typename... Args> T &AddComponent(Args &&...args) {
    if constexpr (HasToString<T>) {
      if (HasComponent<T>()) {

        return GetComponent<T>();
      }
    }
    return scene_->m_Registry.emplace<T>(m_EntityHandle,
                                         std::forward<Args>(args)...);
  }

  template <typename T> bool HasComponent() {
    return scene_->m_Registry.all_of<T>(m_EntityHandle);
  }

  template <typename T> void RemoveComponent() {
    if (!HasComponent<T>()) {
      return;
    }
    scene_->m_Registry.remove<T>(m_EntityHandle);
  }

  template <typename T> T &GetComponent() {
    if (!HasComponent<T>()) {
      throw std::runtime_error("Component not found: " +
                               std::string(typeid(T).name()));
    }
    return scene_->m_Registry.get<T>(m_EntityHandle);
  }

  EntityID GetID() { return GetComponent<IDComponent>().ID; }

  operator entt::entity() const { return m_EntityHandle; }

private:
  entt::entity m_EntityHandle{entt::null};
  Scene *scene_ = nullptr;
};
} // namespace engine
