// core/FireSimulationSystem.cpp
#include "FireSimulationSystem.h"
#include "core/simulation.cuh"  // SimulationStep(...)

// Конструктор — можно потом добавить загрузку конфига из файла
FireSimulationSystem::FireSimulationSystem() = default;

void FireSimulationSystem::Step(Agent* agents_gpu, int count, float dt) {
  if ((agents_gpu == nullptr) || count <= 0) {
    return;
  }

  // Собираем "готовые" параметры для ядра из доменного конфига
  FireSimulationParams params{};
  BuildSimulationParams(m_config, params);

  // Вектор ветра берём из конфига / внешнего кода
  // (можно синхронизировать с твоим WindConfig)
  glm::vec3 wind = m_wind;

  // Вызываем уже существующую CUDA-функцию
  SimulationStep(agents_gpu, count, dt, wind, params);
}
