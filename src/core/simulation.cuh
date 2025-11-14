#pragma once
#include "agent.h"
#include <cuda_runtime.h>
#include <glm/glm.hpp>
struct curandStateXORWOW;
struct HashEntry {
  int agent_index;
  int next;
};

struct HashTable {
  int *table;         // [table_size]
  HashEntry *entries; // [max_agents]
  int entry_count;
  int table_size;
};

// Глобальные параметры шага симуляции огня(конфиг, а не состояние)
struct FireSimulationParams {
  float base_check_distance = 15.0f; // Поиск соседних агентов (объём)

  float base_spread_distance =
      3.0f; // Радиус распространения от точки к точке между агентами

  float local_spread_distance =
      2.0f; // Радиус распространения внутри одного агента (крона/ствол и т.п.)

  float cross_agent_spread_strength =
      10.0f; // Насколько сильно огонь перекидывается между агентами

  float local_spread_strength = 20.0f; // Насколько сильно огонь “растекается”
                                       // по точкам внутри одного агента

  // На будущее, под физику:
  float humidity_ignition_factor =
      1.5f; // как влажность повышает порог воспламенения
  float humidity_heat_dampen = 0.8f; // как влажность гасит теплопередачу
  float wind_heat_bonus = 2.0f;      // усиление тепла по ветру
  float base_conductivity = 0.1f;    // базовая теплопроводность материала
  float max_heat_distance = 8.0f;    // макс. расстояние для теплопередачи
};

void SimulationStep(Agent *agents, int count, float dt,
                    const glm::vec3 &wind_force,
                    const FireSimulationParams &params);

inline void SimulationStep(Agent* agents, int count, float dt,
                           const glm::vec3& wind_force) {
    static const FireSimulationParams K_DEFAULT_PARAMS{};
    SimulationStep(agents, count, dt, wind_force, K_DEFAULT_PARAMS);
}