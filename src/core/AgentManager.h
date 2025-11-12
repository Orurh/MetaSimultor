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
};
class AgentManager {
 public:
  AgentManager(int max_agents);
  ~AgentManager();
  Agent CreateAgent(AgentType type, Float3 position,
                    const AgentComponents& components = AgentComponents());

  Agent AddTree(int tree_id, float base_x, float base_z);
  void AddFire(float x, float y, float intensity);
  void AddFireAt(const Float3& world, float intensity);
  void AddAgent(const Agent& angent);
  bool RemoveAgentAt(int idx, int* outSwappedFrom, int* outSwappedTo);
  Agent* getDeviceAgents() { return m_agents; }
  int getAgentCount() const { return m_count; }


Agent GetAgentCPU(int idx) const;

  // Копирует переданного агента с хоста на устройство по индексу
  void  UpdateAgentCPU(int idx, const Agent& a);

  // --- Массовые операции (если удобнее заливать пакетно) ---
  std::vector<Agent> DownloadAllAgentsCPU() const;
  void UploadAllAgentsCPU(const std::vector<Agent>& v);

  
  void IgniteRandomPoint(int agent_index);
  void Clear();
  void SetAgents(const std::vector<Agent>& agents) {
    Clear();
    for (const auto& agent : agents) {
      AddAgent(agent);
    }
  }

 private:
  void generateTreeHeatPoints(FireCollider& collider);
  void setupTreeFireCollider(Agent& tree);
  void setupFireSourceCollider(Agent& fire, float x, float y);

  Agent* m_agents;
  int m_capacity;
  int m_count;
};