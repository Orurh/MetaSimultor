// core/FireSimulationSystem.cpp
#include "FireSimulationSystem.h"
#include "core/simulation.cuh"  // SimulationStep(...)

// Конструктор — можно потом добавить загрузку конфига из файла
FireSimulationSystem::FireSimulationSystem() = default;

void FireSimulationSystem::Step(Agent* agents_gpu, int count, float dt) {
  if ((agents_gpu == nullptr) || count <= 0) {
    return;
  }

  // Собираем параметры симуляции
  FireSimulationParams params{};
  BuildSimulationParams(m_config, params);

  glm::vec3 wind = m_wind;

  const float WIND_SPEED = glm::length(wind);
  params.m_wind_heat_bonus = 1.0F + WIND_SPEED * 0.1F;

  // --- Глобальный грид для spatial-­поиска соседей ---
  FireGridParams grid = m_grid;

  if (!m_hasGrid) {
    // Простой дефолт: квадратный мир вокруг нуля
    grid.cell_size   = params.m_base_check_distance;   // логично брать радиус поиска
    grid.world_min_x = -1000.0f;
    grid.world_min_z = -1000.0f;
    grid.grid_size_x = 256;    // ~ 256 * 15м = 3.8км по X
    grid.grid_size_z = 256;    // то же по Z
  }

  // Основной CUDA-шаг
  SimulationStep(agents_gpu, count, dt, wind, params, grid);
}

