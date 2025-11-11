// scene/SceneObjectStore.h
#pragma once
#include <vector>
#include "SceneObject.h"

namespace engine {

class SceneObjectStore {
public:
  size_t Add(SceneObject obj);          // возвращает индекс
  void   RemoveByIndex(size_t idx);     // swap + pop
  SceneObject*       Get(size_t idx);
  const SceneObject* Get(size_t idx) const;

  const std::vector<SceneObject>& All() const { return m_objs; }

  // Поиск ближайшего по XZ
  int FindClosestXZ(glm::vec3 world, float maxXZDist) const;

private:
  std::vector<SceneObject> m_objs;
};

} // namespace engine
