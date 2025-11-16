// core/fire/FireModule.h
#pragma once

#include "FireMaterialLibrary.h"
#include "FireSimulationSystem.h"
#include "core/AgentManager.h"

#include <glm/glm.hpp>
#include <string_view>

class FireModule {
public:
  FireModule();

  FireConfig &Config() { return m_sim.Config(); }
  const FireConfig &Config() const { return m_sim.Config(); }

  FireSimulationSystem &Simulation() { return m_sim; }
  const FireSimulationSystem &Simulation() const { return m_sim; }

  FireMaterialLibrary &Materials() { return m_materials; }
  const FireMaterialLibrary &Materials() const { return m_materials; }

  // Один шаг симуляции для текущих агентов
  void Step(AgentManager &manager, float dt, const glm::vec3 &wind);

  // Применить материал к только что созданному агенту
  void ApplyMaterialToAgent(Agent &agent, std::string_view material_id);

private:
  FireSimulationSystem m_sim;
  FireMaterialLibrary m_materials;
};
