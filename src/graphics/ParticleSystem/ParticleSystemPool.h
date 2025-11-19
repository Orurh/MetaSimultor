#include "ParticleSystem.h"
#include <chrono>
#include <deque>
#include <iostream>
#include <vector>
namespace engine {

class ParticleSystemPool {
public:
  // Предварительная инициализация пула
  static void Initialize(size_t initialSize = 100) {
    for (size_t i = 0; i < initialSize; ++i) {
      auto sys = CreateNewSystem();
      pool.push_back(sys);
    }
  }

  static std::shared_ptr<ParticleSystem> AcquireFire() {
    if (!pool.empty()) {
      auto sys = pool.front();
      pool.pop_front();
      ResetSystem(sys); // Важно: сброс состояния!
      return sys;
    }

    // Создаём новую если пул пуст
    return CreateNewSystem();
  }

  static void Release(const std::shared_ptr<ParticleSystem> &sys) {
    if (!sys)
      return;

    sys->Stop();

    // Ограничиваем размер пула (например, максимум 50 систем)
    if (pool.size() < maxPoolSize) {
      pool.push_back(sys);
    } else {
      // Система автоматически уничтожится при выходе из scope
    }
  }

  static size_t GetPoolSize() { return pool.size(); }

private:
  static std::shared_ptr<ParticleSystem> CreateNewSystem() {
    auto sys = std::make_shared<ParticleSystem>();

    sys->SetFireEffect();

    sys->Stop();
    return sys;
  }

  static void ResetSystem(const std::shared_ptr<ParticleSystem> &sys) {
    sys->Stop();
    // Если в ParticleSystem есть метод сброса - вызываем его
    // sys->Reset();
  }

  inline static std::deque<std::shared_ptr<ParticleSystem>> pool;
  inline static const size_t maxPoolSize = 1000;
};

} // namespace engine