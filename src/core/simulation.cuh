#pragma once

#include <cuda_runtime.h>
#include <glm/glm.hpp>
#include "agent.h"
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

// Выполняет один шаг симуляции огня для массива агентов на GPU.
void SimulationStep(Agent* agents, int count, float dt, const glm::vec3& wind_force,
                    const FireSimulationParams& params);

// Упрощённый вариант с параметрами по умолчанию.
inline void SimulationStep(Agent* agents, int count, float dt, const glm::vec3& wind_force) {
  static const FireSimulationParams kDefaultParams{};
  SimulationStep(agents, count, dt, wind_force, kDefaultParams);
}
