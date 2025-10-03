#pragma once
#include <cstdint>
#include "cuda_include.h"
enum AgentType {
    Tree = 0,
    Fire = 1,
    NONE = 2
};
struct Agent {
    float pos_x, pos_y;
    float intensity;
    float fuel;
    AgentType type;
    __host__ __device__
     Agent() : pos_x(0), pos_y(0), intensity(0), fuel(0), type(AgentType::Tree) {}
    __host__ __device__
    Agent(float x, float y, float intencity, float fuel, AgentType type) : pos_x(x), pos_y(y)
    , intensity(intencity), fuel(fuel), type(type) {

    }
    
    __host__ __device__ bool isBurning() const { 
        return type == 1 && intensity > 0.1f; 
    }
    
    __host__ __device__ bool isAliveTree() const { 
        return type == 0 && fuel > 0.0f; 
    }
};