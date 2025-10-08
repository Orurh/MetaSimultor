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

    void AddTree(int tree_id, float base_x, float base_z, 
                                              int max_branches = 50.0f);
    void AddFire(float x, float y, float intensity);  
    void AddAgent(const Agent& angent);
    Agent* getDeviceAgents() { return m_agents; }
    int getAgentCount() const { return m_count; }

private:
     void generateBranch(int tree_id, int parent_id,
                              const glm::vec3& start_pos, const BranchConfig& config,
                              int depth, int max_depth);
    
    BranchConfig randomBranchConfig(int depth);
    Agent* m_agents;
    int m_capacity;
    int m_count;
};