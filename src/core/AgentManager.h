#pragma once
#include "agent.h"
#include <glm/glm.hpp>
#include <vector>

struct AgentComponents {
  // Базовые свойства
  float m_intencity = 0.8F;
  int m_object_id = 0;

  // Настройки коллайдера
  Float3 m_collider_bounds_min = {-1, 0, -1};
  Float3 m_collider_bounds_max = {1, 2, 1};
  int m_collider_point_count = 10;
  float m_collider_fuel_density = 20.0F;
  float m_scale = 1.0F;
};
class AgentManager {
  public:
  AgentManager(int max_agents);
  ~AgentManager();
  // точечный источник огня
  void AddFireAt(const Float3& world, float intensity);
  int AddAgent(const Agent& agent);
  bool RemoveAgentAt(int idx, int* outSwappedFrom, int* outSwappedTo);
  int GetAgentCount() const { return m_count; }
  void Clear();
  void SetAgents(const std::vector<Agent> &agents) {
    Clear();
    for (const auto &agent : agents) {
      AddAgent(agent);
    }
  }
  std::vector<Agent> LoadAgentsFromGPU();

  Agent *GetDeviceAgents() { return m_agents; }

  private:
  Agent *m_agents{};
  int m_capacity;
  int m_count;
};