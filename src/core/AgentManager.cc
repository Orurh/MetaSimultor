#include "AgentManager.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h> 
#include <vector>
#include <random>
#include <iostream>
AgentManager::AgentManager(int max_agents) : m_capacity(max_agents), m_count(0) {
    cudaMalloc(&m_agents, max_agents * sizeof(Agent));
}

AgentManager::~AgentManager() {
    cudaFree(m_agents);
}

void AgentManager::AddTree(int tree_id, float base_x, float base_z, 
                                              int max_branches) {
    Agent trunk;
    trunk.tree_id = tree_id;    
    trunk.pos_x = base_x;
    trunk.pos_z = base_z;
    trunk.pos_y = 0.0f;  
    trunk.type = TREE;
    trunk.fuel = 100.0f;  
    trunk.branch_info.is_trunk = true;
    trunk.branch_info.parent_id = -1;
    trunk.branch_info.length = 0.1f;
    trunk.branch_info.thickness = 2.0f;
    
    AddAgent(trunk);
    
    BranchConfig trunk_config{5.0f, 0.0f, 4, 0.7f};
    generateBranch(tree_id, m_count-1, glm::vec3(base_x, 0.5, base_z), trunk_config, 0, 4);
}
void AgentManager::AddFire(float x, float y, float intencity) {
    if(m_capacity <= m_count) return;
    Agent fire(x, y, intencity, 100.0f, 100.0f, AgentType::FIRE, -1);
    cudaMemcpy(&m_agents[m_count], &fire, sizeof(Agent), cudaMemcpyHostToDevice);
    m_count++;
}

void AgentManager::AddAgent(const Agent &agent)
{
    if (m_count >= m_capacity) {
        std::cerr << "AgentManager: capacity exceeded!" << std::endl;
        return;
    }
    
    cudaMemcpy(m_agents + m_count, &agent, sizeof(Agent), cudaMemcpyHostToDevice);
    m_count++;
}
void AgentManager::generateBranch(int tree_id, int parent_id, const glm::vec3 &start_pos, const BranchConfig &config, int depth, int max_depth)
{
    if(depth >= max_depth || m_count >= m_capacity) return;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angle_dist(0.0f, 360.0f); // Полный круг 0-360°
    std::uniform_real_distribution<float> tilt_dist(15.0f, 45.0f);  // Наклон веток
    std::uniform_real_distribution<float> length_dist(0.3f, 0.8f);

    for(int i = 0; i < config.children_count; ++i) {
        if(m_count >= m_capacity) return;
        
        float horizontal_angle = angle_dist(gen); // Горизонтальный угол (вокруг ствола)
        float vertical_tilt = tilt_dist(gen);     // Вертикальный наклон
        float len = config.length * length_dist(gen);

        // Конвертируем в радианы
        float rad_horizontal = glm::radians(horizontal_angle);
        float rad_vertical = glm::radians(vertical_tilt);

        // Вычисляем позицию конца ветки (сферические координаты)
        glm::vec3 end_pos = start_pos + glm::vec3(
            sin(rad_horizontal) * sin(rad_vertical) * len,  // X
            cos(rad_vertical) * len,                        // Y (вверх)
            cos(rad_horizontal) * sin(rad_vertical) * len   // Z
        );

        Agent branch;
        branch.tree_id = tree_id;
        branch.pos_x = end_pos.x;
        branch.pos_y = end_pos.y;
        branch.pos_z = end_pos.z;
        branch.type = TREE_BRANCH;
        branch.fuel = 20.0f + (max_depth - depth) * 5.0f;
        branch.intensity = 0.0f; // ← ВАЖНО: инициализируй intensity!
        branch.branch_info.is_trunk = false;
        branch.branch_info.parent_id = parent_id;
        branch.branch_info.length = len;
        branch.branch_info.thickness = config.thickness_factor * (1.0f - depth * 0.2f);

        AddAgent(branch);

        if(depth < max_depth - 1) {
            BranchConfig child_config = randomBranchConfig(depth + 1);
            generateBranch(tree_id, m_count-1, end_pos, child_config, depth + 1, max_depth);
        }
    }
}

BranchConfig AgentManager::randomBranchConfig(int depth)
 {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> length_dist(1.0f, 3.0f);
        std::uniform_int_distribution<int> count_dist(1, 3);

        return BranchConfig{
            length_dist(gen) * (1.0f - depth * 0.2f), 
            0.0f,
            count_dist(gen),
            0.6f - depth * 0.1f
        };
}
