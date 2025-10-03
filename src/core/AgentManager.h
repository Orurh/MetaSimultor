#include "agent.h"
class AgentManager {
public:
    AgentManager(int max_agents);
    ~AgentManager();

    void AddTree(float x, float y, float fuel_amount = 50.0f);
    void AddFire(float x, float y, float intensity);  // Исправлено опечатку
    Agent* getDeviceAgents() { return m_agents; }
    int getAgentCount() const { return m_count; }

private:
    Agent* m_agents;
    int m_capacity;
    int m_count;
};