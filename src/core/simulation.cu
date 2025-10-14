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

__global__ void update_agents_kernel(Agent* agents, int count, float dt, float3 wind_force) {
   int idx = threadIdx.x + blockIdx.x * blockDim.x;
    if (idx >= count) return;
    
    Agent* agent = &agents[idx];
    
    // Нормализуем вектор ветра и вычисляем его силу
    float wind_strength = sqrtf(wind_force.x * wind_force.x + 
                               wind_force.y * wind_force.y + 
                               wind_force.z * wind_force.z);
    float3 wind_dir = {0, 0, 0};
    if (wind_strength > 0.001f) {
        wind_dir = {wind_force.x / wind_strength, 
                   wind_force.y / wind_strength, 
                   wind_force.z / wind_strength};
    }
    
    // 1. Обновляем горение с учетом ветра
    for (int i = 0; i < agent->fire_collider.point_count; i++) {
        if (agent->fire_collider.intensities[i] > 0.1f) {
            // Ветер ускоряет горение
            float wind_burn_effect = 1.0f + wind_strength * 0.5f;
            agent->fire_collider.fuels[i] -= agent->fire_collider.intensities[i] * dt * 5.0f * wind_burn_effect;
            
            // Ветер влияет на интенсивность
            float wind_intensity_effect = 1.0f + wind_strength * 0.3f;
            agent->fire_collider.intensities[i] += dt * 2.0f * wind_intensity_effect;
            
            if (agent->fire_collider.intensities[i] > 1.0f) {
                agent->fire_collider.intensities[i] = 1.0f;
            }
            
            if (agent->fire_collider.fuels[i] <= 0.0f) {
                agent->fire_collider.intensities[i] = 0.0f;
                agent->fire_collider.fuels[i] = 0.0f;
            }
        }
    }
    
    // 2. Распространение огня между агентами с улучшенным учетом ветра
    for (int i = 0; i < agent->fire_collider.point_count; i++) {
        if (agent->fire_collider.intensities[i] > 0.3f) {
            glm::vec3 point_pos = glm::vec3(
                agent->fire_collider.heat_points[i].x,
                agent->fire_collider.heat_points[i].y,
                agent->fire_collider.heat_points[i].z
            );
            
            for (int other_idx = 0; other_idx < count; other_idx++) {
                if (other_idx == idx) continue;
                
                Agent* other_agent = &agents[other_idx];
                
                // Динамическое расстояние проверки в зависимости от силы ветра
                float max_check_distance = 15.0f + wind_strength * 10.0f; // Ветер увеличивает дальность
                
                // Проверяем расстояние между bounding box'ами
                float3 agent_center = {
                    (agent->fire_collider.bounds_min.x + agent->fire_collider.bounds_max.x) * 0.5f,
                    (agent->fire_collider.bounds_min.y + agent->fire_collider.bounds_max.y) * 0.5f,
                    (agent->fire_collider.bounds_min.z + agent->fire_collider.bounds_max.z) * 0.5f
                };
                
                float3 other_center = {
                    (other_agent->fire_collider.bounds_min.x + other_agent->fire_collider.bounds_max.x) * 0.5f,
                    (other_agent->fire_collider.bounds_min.y + other_agent->fire_collider.bounds_max.y) * 0.5f,
                    (other_agent->fire_collider.bounds_min.z + other_agent->fire_collider.bounds_max.z) * 0.5f
                };
                
                float box_dist = distance3d(agent_center.x, agent_center.y, agent_center.z,
                                          other_center.x, other_center.y, other_center.z);
                
                // Учитываем направление ветра при проверке расстояния
                if (wind_strength > 0.1f) {
                    float3 to_other_dir = {
                        other_center.x - agent_center.x,
                        other_center.y - agent_center.y,
                        other_center.z - agent_center.z
                    };
                    float to_other_length = sqrtf(to_other_dir.x * to_other_dir.x + 
                                                 to_other_dir.y * to_other_dir.y + 
                                                 to_other_dir.z * to_other_dir.z);
                    
                    if (to_other_length > 0.001f) {
                        to_other_dir.x /= to_other_length;
                        to_other_dir.y /= to_other_length;
                        to_other_dir.z /= to_other_length;
                        
                        // Увеличиваем дальность по направлению ветра
                        float wind_dot = to_other_dir.x * wind_dir.x + 
                                        to_other_dir.y * wind_dir.y + 
                                        to_other_dir.z * wind_dir.z;
                        
                        if (wind_dot > 0.5f) {
                            max_check_distance += wind_strength * 20.0f * wind_dot;
                        }
                    }
                }
                
                if (box_dist < max_check_distance) {
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
                        
                        // Динамическое расстояние распространения
                        float max_spread_distance = 3.0f + wind_strength * 2.0f;
                        
                        if (point_dist < max_spread_distance) {
                            glm::vec3 to_other = other_point_pos - point_pos;
                            glm::vec3 to_other_normalized = glm::normalize(to_other);
                            glm::vec3 wind_vec = glm::vec3(wind_force.x, wind_force.y, wind_force.z);
                            
                            float wind_effect = glm::dot(to_other_normalized, glm::normalize(wind_vec));
                            wind_effect = (wind_effect + 1.0f) * 0.5f; // Приводим к 0-1
                            
                            // Сильный ветер значительно увеличивает шанс распространения
                            float wind_multiplier = 1.0f + wind_strength * 3.0f * wind_effect;
                            
                            float distance_factor = 1.0f - point_dist / max_spread_distance;
                            float spread_prob = agent->fire_collider.intensities[i] 
                                              * distance_factor 
                                              * dt * 10.0f
                                              * wind_multiplier;
                            
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

void SimulationStep(Agent* agents, int count, float dt, const glm::vec3& wind_force) {
    cudaDeviceSynchronize();
    dt*= 0.1;
    const int block_size = 256;
    const int grid_size = (count + block_size - 1) / block_size;
    float3 f = {wind_force.x, wind_force.y, wind_force.z};
    update_agents_kernel<<<grid_size, block_size>>>(agents, count, dt, f);
    cudaDeviceSynchronize();
    
    // Проверка ошибок CUDA
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        printf("CUDA error: %s\n", cudaGetErrorString(err));
    }
}