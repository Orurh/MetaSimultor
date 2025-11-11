// scene/SceneObjectStore.cc
#include "SceneObjectStore.h"
#include <utility> // std::swap

namespace engine {

size_t SceneObjectStore::Add(SceneObject obj) {
  m_objs.push_back(std::move(obj));
  return m_objs.size() - 1;
}

void SceneObjectStore::RemoveByIndex(size_t idx) {
  if (idx >= m_objs.size()) return;
  if (idx != m_objs.size() - 1) {
    std::swap(m_objs[idx], m_objs.back());
  }
  m_objs.pop_back();
}

SceneObject* SceneObjectStore::Get(size_t idx) {
  if (idx >= m_objs.size()) return nullptr;
  return &m_objs[idx];
}

const SceneObject* SceneObjectStore::Get(size_t idx) const {
  if (idx >= m_objs.size()) return nullptr;
  return &m_objs[idx];
}

int SceneObjectStore::FindClosestXZ(glm::vec3 world, float maxXZDist) const {
  int   best   = -1;
  float bestD2 = maxXZDist * maxXZDist;

  for (int i = 0; i < static_cast<int>(m_objs.size()); ++i) {
    const auto& o = m_objs[i];
    if (!o.is_visible) continue;
    const float dx = o.position.x - world.x;
    const float dz = o.position.z - world.z;
    const float d2 = dx*dx + dz*dz;
    if (d2 <= bestD2) { bestD2 = d2; best = i; }
  }
  return best;
}

} // namespace engine
