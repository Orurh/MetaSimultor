#include "simulation.cuh"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cmath>
#include <iostream>
#include <glm/glm.hpp>

__device__ float distance3d(float x1, float y1, float z1, float x2, float y2, float z2) {
    float dx = x1 - x2;
    float dy = y1 - y2;
    float dz = z1 - z2;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

__global__ void update_agents_kernel(Agent* agents, int count, float dt) {
    int idx = threadIdx.x + blockIdx.x * blockDim.x;
    
    if (idx >= count) return;
    
    Agent* agent = &agents[idx];
    
    // 1. Обновляем горение для каждого агента
    for (int i = 0; i < agent->fire_collider.point_count; i++) {
        if (agent->fire_collider.intensities[i] > 0.1F) {
            // Точка горит - уменьшаем топливо
            agent->fire_collider.fuels[i] -= agent->fire_collider.intensities[i] * dt * 5.0f;
            
            // Увеличиваем интенсивность со временем (огонь разгорается)
            agent->fire_collider.intensities[i] += dt * 2.0f;
            if (agent->fire_collider.intensities[i] > 1.0f) {
                agent->fire_collider.intensities[i] = 1.0f;
            }
            
            // Если топливо кончилось - тушим
            if (agent->fire_collider.fuels[i] <= 0.0f) {
                agent->fire_collider.intensities[i] = 0.0f;
                agent->fire_collider.fuels[i] = 0.0f;
            }
        }
    }
    
    // 2. Распространение огня между точками разных агентов
    for (int i = 0; i < agent->fire_collider.point_count; i++) {
        if (agent->fire_collider.intensities[i] > 0.3f) {
            // Эта точка достаточно сильно горит для распространения
            
            glm::vec3 point_pos = glm::vec3(
                agent->fire_collider.heat_points[i].x,
                agent->fire_collider.heat_points[i].y,
                agent->fire_collider.heat_points[i].z
            );
            
            // Ищем другие агенты для распространения огня
            for (int other_idx = 0; other_idx < count; other_idx++) {
                if (other_idx == idx) continue;
                
                Agent* other_agent = &agents[other_idx];
                
                // Проверяем расстояние между bounding box'ами
                float box_dist = distance3d(
                    (agent->fire_collider.bounds_min.x + agent->fire_collider.bounds_max.x) * 0.5f,
                    (agent->fire_collider.bounds_min.y + agent->fire_collider.bounds_max.y) * 0.5f,
                    (agent->fire_collider.bounds_min.z + agent->fire_collider.bounds_max.z) * 0.5f,
                    (other_agent->fire_collider.bounds_min.x + other_agent->fire_collider.bounds_max.x) * 0.5f,
                    (other_agent->fire_collider.bounds_min.y + other_agent->fire_collider.bounds_max.y) * 0.5f,
                    (other_agent->fire_collider.bounds_min.z + other_agent->fire_collider.bounds_max.z) * 0.5f
                );
                
                if (box_dist < 15.0f) { 
                    // Проверяем все точки в другом агенте
                    for (int j = 0; j < other_agent->fire_collider.point_count; j++) {
                        if (other_agent->fire_collider.intensities[j] > 0.1f) continue; 
                        
                        glm::vec3 other_point_pos = glm::vec3(
                            other_agent->fire_collider.heat_points[j].x,
                            other_agent->fire_collider.heat_points[j].y,
                            other_agent->fire_collider.heat_points[j].z
                        );
                        
                        float point_dist = distance3d(
                            point_pos.x, point_pos.y, point_pos.z,
                            other_point_pos.x, other_point_pos.y, other_point_pos.z
                        );
                        
                        if (point_dist < 3.0f) { 
                            float spread_prob = agent->fire_collider.intensities[i] * (1.0f - point_dist / 3.0f) * dt * 10.0f;
                            
                            unsigned int seed = idx + other_idx + i + j;
                            float random_val = (float)((seed * 9301 + 49297) % 233280) / 233280.0f;
                            
                            if (random_val < spread_prob) {
                                other_agent->fire_collider.intensities[j] = 0.2f; 
                            }
                        }
                    }
                }
            }
        }
    }
    
    for (int i = 0; i < agent->fire_collider.point_count; i++) {
        if (agent->fire_collider.intensities[i] > 0.1f) {
            glm::vec3 point_pos = glm::vec3(
                agent->fire_collider.heat_points[i].x,
                agent->fire_collider.heat_points[i].y,
                agent->fire_collider.heat_points[i].z
            );
            
            for (int j = 0; j < agent->fire_collider.point_count; j++) {
                glm::vec3 other_point_pos = glm::vec3(
                    agent->fire_collider.heat_points[j].x,
                    agent->fire_collider.heat_points[j].y,
                    agent->fire_collider.heat_points[j].z
                );
                
                float point_dist = distance3d(
                    point_pos.x, point_pos.y, point_pos.z,
                    other_point_pos.x, other_point_pos.y, other_point_pos.z
                );
                
                if (point_dist < 2.0f) {
                    float spread_prob = agent->fire_collider.intensities[i] * (1.0f - point_dist / 2.0f) * dt * 20.0f;
                    
                    unsigned int seed = idx + i + j;
                    float random_val = (float)((seed * 9301 + 49297) % 233280) / 233280.0f;
                    
                    if (random_val < spread_prob) {
                        agent->fire_collider.intensities[j] = 0.3f;
                    }
                }
            }
        }
    }
    
    // 4. Обновляем тип агента на основе точек горения
    bool all_burned = true;
    
    for (int i = 0; i < agent->fire_collider.point_count; i++) {
        if (agent->fire_collider.intensities[i] > 0.1f) {
            all_burned = false;
        }
        if (agent->fire_collider.fuels[i] > 0.1f) {
            all_burned = false;
        }
    }
    
    if (all_burned && agent->type == TREE) {
        agent->type = BURNED;
    }
}

void SimulationStep(Agent* agents, int count, float dt) {
    cudaDeviceSynchronize();
    
    const int block_size = 256;
    const int grid_size = (count + block_size - 1) / block_size;
    
    update_agents_kernel<<<grid_size, block_size>>>(agents, count, dt);
    cudaDeviceSynchronize();
    
    // Проверка ошибок CUDA
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        printf("CUDA error: %s\n", cudaGetErrorString(err));
    }
}