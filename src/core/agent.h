#pragma once
#include <cstdint>
#include "cuda_include.h"

enum AgentType {
    TREE = 0,
    TREE_BRANCH = 1,  // Ветка дерева
    FIRE = 2,         // Огонь на ветке
    BURNED = 3        // Сгоревшая ветка
};

struct TreeBranch {
    float length;
    float thickness;
    int parent_id;     // ID родительской ветки (-1 для ствола)
    bool is_trunk;     // Это ствол?
};

struct Agent {
    float pos_x, pos_y, pos_z;  // 3D позиция
    float intensity;            // Интенсивность огня
    float fuel;                 // Количество топлива
    AgentType type;
    int tree_id;               // ID дерева для группировки веток
    TreeBranch branch_info;     // Информация о ветке
    
    __host__ __device__
    Agent() : pos_x(0), pos_y(0), pos_z(0), intensity(0), fuel(0), 
              type(AgentType::TREE_BRANCH), tree_id(-1), branch_info() {}
    
    __host__ __device__
    Agent(float x, float y, float z, float intensity, float fuel, 
          AgentType type, int tree_id) 
        : pos_x(x), pos_y(y), pos_z(z), intensity(intensity), fuel(fuel),
          type(type), tree_id(tree_id), branch_info() {}
    
    __host__ __device__
    bool isBurning() const { 
        return type == FIRE && intensity > 0.1f; 
    }
    
    __host__ __device__
    bool hasFuel() const { 
        return fuel > 0.0f; 
    }
    
    __host__ __device__
    bool canIgnite() const {
        return type == TREE_BRANCH && fuel > 0.0f;
    }
};