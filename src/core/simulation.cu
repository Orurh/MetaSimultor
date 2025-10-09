#include "simulation.cuh"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cmath>
#include <iostream>

__global__ void debug_kernel() {
    printf("Hello from GPU! threadIdx.x = %d, blockIdx.x = %d\n", 
           threadIdx.x, blockIdx.x);
}

__global__ void update_agents_kernel(Agent* agents, int count, float dt) {
    // int idx = threadIdx.x + blockIdx.x * blockDim.x;
    
    // if (idx >= count) return;
    
    // Agent* agent = &agents[idx];
    
    // if (agent->type == AgentType::TREE) { 
    //     int burning_neighbors = 0;
    //     for (int i = 0; i < count; i++) {
    //         if (i == idx) continue;
            
    //         Agent* other = &agents[i];
    //         if (other->type == AgentType::FIRE) { 

    //             float dx = agent->pos_x - other->pos_x;
    //             float dy = agent->pos_y - other->pos_y;
    //             float distance = sqrtf(dx*dx + dy*dy);
                
    //             if (distance < 10.0f) {
    //                 burning_neighbors++;
    //             }
    //         }
    //     }
        
    //     if (burning_neighbors >= 1) {
    //         agent->type = AgentType::FIRE; 
    //         agent->intensity = 1.0f;
    //         agent->fuel = 30.0f;
    //     }
    // }
    // else if (agent->type == 1) { 
    //     agent->fuel -= dt * 8.0f;
    //     agent->intensity = agent->fuel / 30.0f;
        
    //     if (agent->fuel <= 0.0f) {
    //         agent->type = AgentType::BURNED;
    //         agent->intensity = 0.0f;
    //     }
    // }
}

void SimulationStep(Agent* agents, int count, float dt) {
    cudaDeviceSynchronize();
    
    const int block_size = 256;
    const int grid_size = (count + block_size - 1) / block_size;
    
    
    update_agents_kernel<<<grid_size, block_size>>>(agents, count, dt);
    cudaDeviceSynchronize();
}