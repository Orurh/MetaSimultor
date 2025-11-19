#pragma once
#include "agent.h"
#include <cuda_runtime.h>
#include <glm/glm.hpp>
#include "fire/FireSimulationParams.h"

// Простые структуры для будущего spatial hashing (пока не используются).
struct curandStateXORWOW;

struct HashEntry {
  int agent_index;
  int next;
};

struct HashTable {
  int* table;          // [table_size]
  HashEntry* entries;  // [max_agents]
  int entry_count;
  int table_size;
};

struct FireGridParams {
  float world_min_x;
  float world_min_z;
  float cell_size;
  int grid_size_x;
  int grid_size_z;
};

// Выполняет один шаг симуляции огня для массива агентов на GPU.
void SimulationStep(Agent* agents, int count, float dt, const glm::vec3& wind_force,
                    const FireSimulationParams& params, const FireGridParams& grid);

// // Упрощённый вариант с параметрами по умолчанию.
// inline void SimulationStep(Agent* agents, int count, float dt, const glm::vec3& wind_force) {
//   static const FireSimulationParams kDefaultParams{};

//   // Примитивный дефолтный грид: большой квадрат вокруг (0,0),
//   // размер ячейки ~ базовой дистанции распространения.
//   FireGridParams grid{};
//   grid.world_min_x  = -1000.0f;
//   grid.world_min_z  = -1000.0f;
//   grid.cell_size    = kDefaultParams.m_base_check_distance;
//   grid.grid_size_x  = 256;  // 256 * cell_size ≈ 256 * 15 = 3840 метров по X
//   grid.grid_size_z  = 256;  // аналогично по Z

//   SimulationStep(agents, count, dt, wind_force, kDefaultParams, grid);
// }
