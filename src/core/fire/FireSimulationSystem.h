#pragma once

#include "FireConfig.h"
#include "core/agent.h"
#include "core/simulation.cuh"   // <-- чтобы видеть FireGridParams
#include <glm/glm.hpp>

class FireSimulationSystem {
public:
  FireSimulationSystem();

  FireConfig &Config() { return m_config; }
  [[nodiscard]] const FireConfig &Config() const { return m_config; }

  void SetWind(const glm::vec3 &wind) { m_wind = wind; }
  [[nodiscard]] const glm::vec3 &Wind() const { return m_wind; }

  // Scene может один раз настроить грид (по размерам террейна, например)
  void SetGridParams(const FireGridParams& grid) {
    m_grid      = grid;
    m_hasGrid   = true;
  }

  // Один шаг симуляции. Agents должны жить уже на GPU.
  void Step(Agent *agents_gpu, int count, float dt);

private:
  FireConfig m_config;
  glm::vec3  m_wind{1.0F, 0.0F, 0.0F};

  FireGridParams m_grid{};
  bool           m_hasGrid{false};
};
