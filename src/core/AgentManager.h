#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "agent.h"

struct AgentComponents {
  // Базовые свойства
  float intencity = 0.8f;
  int object_id = 0;

  // Настройки коллайдера
  Float3 collider_bounds_min = {-1, 0, -1};
  Float3 collider_bounds_max = {1, 2, 1};
  int collider_point_count = 10;
  float collider_fuel_density = 20.0f;
  float scale = 1.0f;
};
class AgentManager {
 public:
  AgentManager(int max_agents);
  ~AgentManager();
  // точечный источник огня
  void AddFireAt(const Float3& world, float intensity);
  int AddAgent(const Agent& angent);
  bool RemoveAgentAt(int idx, int* outSwappedFrom, int* outSwappedTo);
  int GetAgentCount() const { return m_count; }
  void Clear();
  void SetAgents(const std::vector<Agent>& agents) {
    Clear();
    for (const auto& agent : agents) {
      AddAgent(agent);
    }
  }
  std::vector<Agent> LoadAgentsFromGPU();

  Agent* GetDeviceAgents() { return m_agents; }

 private:
  Agent* m_agents{};
  int m_capacity;
  int m_count;
};