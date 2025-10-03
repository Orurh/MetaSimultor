#include "AgentManager.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h> 
AgentManager::AgentManager(int max_agents) : m_capacity(max_agents), m_count(0) {
    cudaMalloc(&m_agents, max_agents * sizeof(Agent));
}

AgentManager::~AgentManager() {
    cudaFree(m_agents);
}

void AgentManager::AddTree(float x, float y, float fuel_amount ) {
    if(m_capacity <= m_count) return;
    Agent tree(x, y, 0.0f, fuel_amount, AgentType::Tree);
    cudaMemcpy(&m_agents[m_count], &tree, sizeof(Agent), cudaMemcpyHostToDevice);
    m_count++;
}
void AgentManager::AddFire(float x, float y, float intencity) {
    if(m_capacity <= m_count) return;
    Agent fire(x, y, intencity, 100.0f, AgentType::Fire);
    cudaMemcpy(&m_agents[m_count], &fire, sizeof(Agent), cudaMemcpyHostToDevice);
    m_count++;
}
   