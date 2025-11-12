// scene/SceneObjectStore.h
#pragma once
#include <glm/vec3.hpp>
#include <vector>
#include "SceneObject.h"

namespace engine {

class SceneObjectStore {
 public:
  size_t Add(SceneObject obj);
  void RemoveByIndex(size_t idx);
  SceneObject* Get(size_t idx);
  [[nodiscard]] const SceneObject* Get(size_t idx) const;

  [[nodiscard]] const std::vector<SceneObject>& All() const { return m_objs; }
  std::vector<SceneObject>& All() { return m_objs; }

  // Поиск ближайшего по XZ
  [[nodiscard]] int FindClosestXZ(glm::vec3 world, float maxXZDist) const;
  void Clear() { m_objs.clear(); }
  [[nodiscard]] bool Empty() const { return m_objs.empty(); }
  [[nodiscard]] size_t Size() const { return m_objs.size(); }

 private:
  std::vector<SceneObject> m_objs;
};

}  // namespace engine
