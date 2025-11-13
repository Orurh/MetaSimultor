#pragma once
#include "SceneObject.h" // тут объявлен struct ObjectModels (normal/burning/burned)
#include <memory>
#include <string>
#include <unordered_map>

namespace engine {

class ObjectModelStore {
public:
  bool Has(const std::string &id) const { return m_map.contains(id); }

  const ObjectModels *Find(const std::string &id) const {
    auto it = m_map.find(id);
    return it == m_map.end() ? nullptr : &it->second;
  }
  ObjectModels *Find(const std::string &id) {
    auto it = m_map.find(id);
    return it == m_map.end() ? nullptr : &it->second;
  }

  void Put(const std::string &id, ObjectModels models) {
    m_map[id] = std::move(models);
  }

  void Clear() {
    for (auto &[name, models] : m_map) {
      models.Cleanup();
    }
    m_map.clear();
  }

private:
  std::unordered_map<std::string, ObjectModels> m_map;
};

} // namespace engine
