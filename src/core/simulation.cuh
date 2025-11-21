#pragma once
#include <cuda_runtime.h>
#include <glm/glm.hpp>
#include "agent.h"
#include "fire/FireSimulationParams.h"

// Простые структуры для будущего spatial hashing (пока не используются).
struct curandStateXORWOW;

struct HashEntry {
  int m_agent_index;
  int m_next;
};

struct HashTable {
  int* m_table;          // [table_size]
  HashEntry* m_entries;  // [max_agents]
  int m_entry_count;
  int m_table_size;
};

struct FireGridParams {
  float m_world_min_x;
  float m_world_min_z;
  float m_cell_size;
  int m_grid_size_x;
  int m_grid_size_z;
};

// Выполняет один шаг симуляции огня для массива агентов на GPU.
void SimulationStep(Agent* agents, int count, float dt, const glm::vec3& wind_force, const FireSimulationParams& params,
                    const FireGridParams& grid);
