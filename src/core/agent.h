#pragma once
#include <glm/glm.hpp>

#include "cuda_include.h"
enum class FireAgentState : uint8_t {
    Normal  = 0,
    Burning = 1,
    Burned  = 2,
};

struct Float3 {
    float x, y, z;
    
    __host__ __device__ Float3() : x(0), y(0), z(0) {}
    __host__ __device__ Float3(float x, float y, float z) : x(x), y(y), z(z) {}
    __host__ __device__ static Float3 fromGlm(const glm::vec3& v) {
        return Float3(v.x, v.y, v.z);
    }
    
    __host__ __device__ glm::vec3 toGlm() const {
        return glm::vec3(x, y, z);
    }
    __host__ __device__ float& operator[](int i) { 
        return i == 0 ? x : (i == 1 ? y : z); 
    }
    __host__ __device__ float operator[](int i) const { 
        return i == 0 ? x : (i == 1 ? y : z); 
    }
};

struct FireCollider {
    Float3 bounds_min, bounds_max;
    Float3 heat_points[64];
    float fuels[64];
    float intensities[64];   // "мощность" горения (визуал/источник тепла)
    float temperatures[64];  // температура точки (K или °C — главное, везде одинаково)
    float moisture[64];      // локальная влажность (0..1), пока не используем, но пригодится
    float max_temperature; // максимальная температура

        // Материал / физические свойства
    float conductivity;      // базовая теплопроводность коллайдера (дерево/трава)
    float ignition_temp;     // базовый порог воспламенения материала

    int point_count;            

    __host__ __device__
    FireCollider()
        : conductivity(0.1f),
          ignition_temp(600.0f),
          point_count(0)
    {
        for (int k = 0; k < 3; ++k) {
            bounds_min[k] = 0.0f;
            bounds_max[k] = 0.0f;
        }
        for (int i = 0; i < 64; ++i) {
            fuels[i]        = 0.0f;
            intensities[i]  = 0.0f;
            temperatures[i] = 293.15f; // ~20°C
            moisture[i]     = 0.3f;    // немного влажности по умолчанию
        }
    }
};

struct Agent {
    Float3 position;
    FireAgentState type;   
    float intensity{};
    int object_id; 
    
    // Универсальный коллайдер для огня
    FireCollider fire_collider;
    
    __host__ __device__
    Agent() : position(), type(FireAgentState::Normal), object_id(-1) {}
    
    __host__ __device__
    Agent(float x, float y, float z, FireAgentState type, int id) 
        : position(x, y, z), type(type), object_id(id) {}
    
    __host__ __device__
    bool isBurning() const { 
        for (int i = 0; i < fire_collider.point_count; i++) {
            if (fire_collider.intensities[i] > 0.1f) {
                return true;  
            }
        }
        return false;
    }
    
    __host__ __device__
    float getOverallIntensity() const {
        float max = 0.0f;
        for (int i = 0; i < fire_collider.point_count; i++) {
            if (fire_collider.intensities[i] > max) {
                max = fire_collider.intensities[i];
            }
        }
        return max;
    }
};
