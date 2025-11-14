#pragma once
#include "graphics/ParticleEffect.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace engine {

class ParticleSystemManager {
public:
  static ParticleSystemManager &Instance() {
    static ParticleSystemManager instance;
    return instance;
  }

  // Регистрация систем частиц как ассетов
  void RegisterSystem(const std::string &name,
                      const std::shared_ptr<ParticleEffect> &effect) {
    m_systems[name] = effect;
    m_systemNames.clear();
    for (const auto &pair : m_systems) {
      m_systemNames.push_back(pair.first);
    }
  }

  void UnregisterSystem(const std::string &name) {
    m_systems.erase(name);
    // Обновляем список имен
    m_systemNames.clear();
    for (const auto &pair : m_systems) {
      m_systemNames.push_back(pair.first);
    }
  }

  // Получение систем
  std::shared_ptr<ParticleEffect> GetSystem(const std::string &name) {
    auto it = m_systems.find(name);
    if (it != m_systems.end()) {
      return it->second;
    }
    return nullptr;
  }

  const std::vector<std::string> &GetSystemNames() const {
    return m_systemNames;
  }

  // Сохранение/загрузка библиотеки систем
  void SaveLibrary(const std::string &path) {
    // TODO: реализовать сохранение в файл
  }

  void LoadLibrary(const std::string &path) {
    // TODO: реализовать загрузку из файла
  }

private:
  ParticleSystemManager() = default;
  ~ParticleSystemManager() = default;

  std::unordered_map<std::string, std::shared_ptr<ParticleEffect>> m_systems;
  std::vector<std::string> m_systemNames;
};

} // namespace engine