#include "agent.h"
#include <glm/glm.hpp>
#include <vector>
struct BranchConfig {
    float length;
    float angle;
    int children_count;
    float thickness_factor;
};

class AgentManager {
public:
    AgentManager(int max_agents);
    ~AgentManager();

    void AddTree(int tree_id, float base_x, float base_z);
    void AddFire(float x, float y, float intensity);  
    void AddAgent(const Agent& angent);
    Agent* getDeviceAgents() { return m_agents; }
    int getAgentCount() const { return m_count; }

private:
    void generateTreeHeatPoints(FireCollider& collider, float base_x, float base_z);
    void setupTreeFireCollider(Agent& tree, float base_x, float base_z);
    void setupFireSourceCollider(Agent& fire, float x, float y);
    
    Agent* m_agents;
    int m_capacity;
    int m_count;
};