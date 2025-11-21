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

