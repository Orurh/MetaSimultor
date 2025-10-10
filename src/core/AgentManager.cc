#include "AgentManager.h"
#include <cuda_runtime.h>
#include <vector>
#include <random>
#include <iostream>

AgentManager::AgentManager(int max_agents) : m_capacity(max_agents), m_count(0) {
    cudaMalloc(&m_agents, max_agents * sizeof(Agent));
}

AgentManager::~AgentManager() {
    cudaFree(m_agents);
}

void AgentManager::AddTree(int tree_id, float base_x, float base_z) {
    Agent tree;
    tree.type = TREE;
    tree.object_id = tree_id;
    tree.pos_x = base_x;
    tree.pos_z = base_z;
    tree.pos_y = 0.0f;
    
    // Настраиваем FireCollider для дерева
    setupTreeFireCollider(tree, base_x, base_z);
    
    AddAgent(tree);
}

void AgentManager::setupTreeFireCollider(Agent& tree, float base_x, float base_z) {
    tree.fire_collider.bounds_min[0] = base_x - 2.0f;
    tree.fire_collider.bounds_min[1] = 0.0f;         
    tree.fire_collider.bounds_min[2] = base_z - 2.0f;
    
    tree.fire_collider.bounds_max[0] = base_x + 2.0f;
    tree.fire_collider.bounds_max[1] = 8.0f;         
    tree.fire_collider.bounds_max[2] = base_z + 2.0f;
    
    generateTreeHeatPoints(tree.fire_collider, base_x, base_z);
}
void AgentManager::IgniteRandomPoint(int agent_index)
 {
    if (agent_index < 0 || agent_index >= m_count) return;
    
    Agent agent;
    cudaMemcpy(&agent, m_agents + agent_index, sizeof(Agent), cudaMemcpyDeviceToHost);
    
    if (agent.fire_collider.point_count > 0) {
        int random_point = rand() % agent.fire_collider.point_count;
        agent.fire_collider.intensities[random_point] = 0.5f;
        cudaMemcpy(m_agents + agent_index, &agent, sizeof(Agent), cudaMemcpyHostToDevice);
    }
}
void AgentManager::generateTreeHeatPoints(FireCollider &collider, float base_x, float base_z)
{
    collider.point_count = 0;
    
    for (float height = 0.5f; height <= 8.0f; height += 0.8f) {
        collider.heat_points[collider.point_count][0] = base_x;
        collider.heat_points[collider.point_count][1] = height;
        collider.heat_points[collider.point_count][2] = base_z;
        collider.fuels[collider.point_count] = 30.0f;
        collider.intensities[collider.point_count] = 0.0f;
        collider.point_count++;
    }
    
    float levels[] = {2.0f, 4.0f, 6.0f, 7.5f};
    float level_radii[] = {1.0f, 1.8f, 2.2f, 1.5f};
    int points_per_level[] = {8, 12, 16, 8};
    
    for (int level = 0; level < 4; level++) {
        float height = levels[level];
        float radius = level_radii[level];
        int points = points_per_level[level];
        
        for (int i = 0; i < points; i++) {
            float angle = (float)i / points * 360.0f;
            float rad = glm::radians(angle);
            
            collider.heat_points[collider.point_count][0] = base_x + cos(rad) * radius;
            collider.heat_points[collider.point_count][1] = height;
            collider.heat_points[collider.point_count][2] = base_z + sin(rad) * radius;
            collider.fuels[collider.point_count] = 20.0f;
            collider.intensities[collider.point_count] = 0.0f;
            collider.point_count++;
            
            if (collider.point_count >= 64) break;
        }
        if (collider.point_count >= 64) break;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist_x(base_x - 2.0f, base_x + 2.0f);
    std::uniform_real_distribution<float> dist_z(base_z - 2.0f, base_z + 2.0f);
    std::uniform_real_distribution<float> dist_y(1.0f, 7.0f);
    
    while (collider.point_count < 50 && collider.point_count < 64) {
        collider.heat_points[collider.point_count][0] = dist_x(gen);
        collider.heat_points[collider.point_count][1] = dist_y(gen);
        collider.heat_points[collider.point_count][2] = dist_z(gen);
        collider.fuels[collider.point_count] = 15.0f;
        collider.intensities[collider.point_count] = 0.0f;
        collider.point_count++;
    }
    
    std::cout << "Generated " << collider.point_count << " points for tree" << std::endl;
}

void AgentManager::AddFire(float x, float y, float intensity) {
    if(m_count >= m_capacity) return;
    
    Agent fire;
    fire.type = FIRE_SOURCE;
    fire.object_id = -1;
    fire.pos_x = x;
    fire.pos_z = y;
    fire.pos_y = 0.0f;
    
    // Простой FireCollider для источника огня
    setupFireSourceCollider(fire, x, y);
    
    // Зажигаем точку
    fire.fire_collider.intensities[0] = intensity;
    
    AddAgent(fire);
}

void AgentManager::setupFireSourceCollider(Agent& fire, float x, float y) {
    fire.fire_collider.bounds_min[0] = x - 0.5f;
    fire.fire_collider.bounds_min[1] = 0.0f;  
    fire.fire_collider.bounds_min[2] = y - 0.5f;
    
    fire.fire_collider.bounds_max[0] = x + 0.5f;
    fire.fire_collider.bounds_max[1] = 2.0f;
    fire.fire_collider.bounds_max[2] = y + 0.5f;
    
    // Одна точка в центре
    fire.fire_collider.point_count = 1;
    fire.fire_collider.heat_points[0][0] = x;
    fire.fire_collider.heat_points[0][1] = 1.0f;
    fire.fire_collider.heat_points[0][2] = y;
    fire.fire_collider.fuels[0] = 50.0f;
    fire.fire_collider.intensities[0] = 0.0f;
}

void AgentManager::AddAgent(const Agent &agent) {
    if (m_count >= m_capacity) {
        std::cerr << "AgentManager: capacity exceeded!" << std::endl;
        return;
    }
    
    cudaMemcpy(m_agents + m_count, &agent, sizeof(Agent), cudaMemcpyHostToDevice);
    m_count++;
}