#pragma once
#include <cstdint>
#include "cuda_include.h"

enum AgentType {
    TREE = 0,
    FIRE_SOURCE = 1,    // Точка горения (бывшая ветка)
    BURNED = 2
};

struct FireCollider {
    float bounds_min[3];  // min x,y,z
    float bounds_max[3];  // max x,y,z  
    float heat_points[12][3]; // 12 точек горения (x,y,z для каждой)
    float fuels[12];         // топливо для каждой точки
    float intensities[12];   // интенсивность для каждой точки
    int point_count;        // сколько точек реально используется
    
    __host__ __device__
    FireCollider() : point_count(0) {
        for (int i = 0; i < 3; i++) {
            bounds_min[i] = 0.0f;
            bounds_max[i] = 0.0f;
        }
    }
};

struct Agent {
    float pos_x, pos_y, pos_z;  // Позиция объекта
    AgentType type;
    float intencity;
    int object_id;              // ID объекта
    
    // Универсальный коллайдер для огня
    FireCollider fire_collider;
    
    __host__ __device__
    Agent() : pos_x(0), pos_y(0), pos_z(0), type(TREE), object_id(-1) {}
    
    __host__ __device__
    Agent(float x, float y, float z, AgentType type, int id) 
        : pos_x(x), pos_y(y), pos_z(z), type(type), object_id(id) {}
    
    __host__ __device__
    bool isBurning() const { 
        for (int i = 0; i < fire_collider.point_count; i++) {
            if (fire_collider.intensities[i] > 0.1f) return true;
        }
        return false;
    }
};