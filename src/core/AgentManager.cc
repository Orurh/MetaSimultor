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
    // Bounding box дерева
    tree.fire_collider.bounds_min[0] = base_x - 2.0f; // min X
    tree.fire_collider.bounds_min[1] = 0.0f;          // min Y  
    tree.fire_collider.bounds_min[2] = base_z - 2.0f; // min Z
    
    tree.fire_collider.bounds_max[0] = base_x + 2.0f; // max X
    tree.fire_collider.bounds_max[1] = 8.0f;          // max Y (высота дерева)
    tree.fire_collider.bounds_max[2] = base_z + 2.0f; // max Z
    
    // Генерируем точки горения в виде "ёлки"
    generateTreeHeatPoints(tree.fire_collider, base_x, base_z);
}

void AgentManager::generateTreeHeatPoints(FireCollider& collider, float base_x, float base_z) {
    collider.point_count = 8; 
    
    // Уровни высоты для точек
    float heights[] = {1.0f, 3.0f, 5.0f, 7.0f};
    float radii[] = {0.5f, 1.0f, 1.5f, 1.0f}; 
    
    int point_index = 0;
    for (int level = 0; level < 4; level++) {
        for (int side = 0; side < 2; side++) {
            float angle = side * 180.0f; 
            float rad = glm::radians(angle);
            
            collider.heat_points[point_index][0] = base_x + cos(rad) * radii[level]; // X
            collider.heat_points[point_index][1] = heights[level];                   // Y
            collider.heat_points[point_index][2] = base_z + sin(rad) * radii[level]; // Z
            
            collider.fuels[point_index] = 25.0f;
            collider.intensities[point_index] = 0.0f;
            
            point_index++;
        }
    }
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