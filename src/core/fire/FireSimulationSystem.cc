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

  // Вектор ветра берём из поля m_wind (его должен установить вызывающий код)
  glm::vec3 wind = m_wind;

  // Пример: усиливаем прогрев по направлению ветра в зависимости от скорости
  const float WIND_SPEED = glm::length(wind);
  params.m_wind_heat_bonus = 1.0F + WIND_SPEED * 0.1F;

  // Вызываем уже существующую CUDA-функцию
  SimulationStep(agents_gpu, count, dt, wind, params);
}

