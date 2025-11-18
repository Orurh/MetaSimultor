// core/FireSimulationSystem.h
#pragma once

#include "FireConfig.h"
#include "core/agent.h" // Agent*
#include <glm/glm.hpp>

// Обёртка над CUDA-симуляцией огня.
// Scene / UI работают с FireConfig и FireSimulationSystem,
// а не напрямую с SimulationStep(...)
class FireSimulationSystem {
public:
  FireSimulationSystem();

  FireConfig &Config() { return m_config; }
  [[nodiscard]] const FireConfig &Config() const { return m_config; }

  

  void SetWind(const glm::vec3 &wind) { m_wind = wind; }
  [[nodiscard]] const glm::vec3 &Wind() const { return m_wind; }

  // Один шаг симуляции. Agents должны жить уже на GPU.
  void Step(Agent *agents_gpu, int count, float dt);

private:
  FireConfig m_config;
  glm::vec3 m_wind{1.0F, 0.0F, 0.0F};
};
