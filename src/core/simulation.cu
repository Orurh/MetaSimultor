#include "agent.h"
#include "simulation.cuh"

#include <cuda_runtime.h>
#include <cuda_runtime_api.h>
#include <device_launch_parameters.h>

#include <iostream>

#include "simulation_grid.cuh"
#include "simulation_math.cuh"
#include "simulation_physics.cuh"
#include "simulation_spread.cuh"

using FireMath::MakeWindData;
using FireMath::WindData;

using FirePhysics::CoolBurnedAgent;
using FirePhysics::IgnitePoints;
using FirePhysics::UpdateAgentType;
using FirePhysics::UpdateBurningPoints;

using FireSpread::SpreadBetweenAgents;
using FireSpread::SpreadWithinAgent;

// -----------------------------------------------------------------------------
// CUDA kernel
// -----------------------------------------------------------------------------

__global__ void UpdateAgentsKernel(Agent* agents, int count, float dt, float3 wind_force, FireSimulationParams params, FireGridParams grid,
                                   const int* cell_heads, const int* cell_next) {
  const int IDX = threadIdx.x + (blockIdx.x * blockDim.x);
  if (IDX >= count) {
    return;
  }

  Agent& agent = agents[IDX];
  if (agent.type == FireAgentState::Burned) {
    FirePhysics::CoolBurnedAgent(agent, dt, params);
    return;
  }

  const WindData WIND = MakeWindData(wind_force);

  UpdateBurningPoints(agent, WIND, dt, params);
  SpreadBetweenAgents(agents, IDX, count, grid, cell_heads, cell_next, WIND, dt, params);
  SpreadWithinAgent(agent, dt, params);
  IgnitePoints(agent, dt, params);
  UpdateAgentType(agent, params);
}

// -----------------------------------------------------------------------------
// Host API (интерфейс не меняем)
// -----------------------------------------------------------------------------

void SimulationStep(Agent* agents, int count, float dt, const glm::vec3& wind_force, const FireSimulationParams& params,
                    const FireGridParams& grid) {
  if ((agents == nullptr) || count <= 0) {
    return;
  }

  constexpr int K_BLOCK_SIZE = 256;
  const int GRID_SIZE = (count + K_BLOCK_SIZE - 1) / K_BLOCK_SIZE;

  const float3 f{wind_force.x, wind_force.y, wind_force.z};

  static int* s_d_cell_heads = nullptr;
  static int* s_d_cell_next = nullptr;
  static int allocated_agents = 0;
  static int allocated_cells = 0;

  const int num_cells = grid.m_grid_size_x * grid.m_grid_size_z;
  if (grid.m_grid_size_x <= 0 || grid.m_grid_size_z <= 0 || num_cells <= 0) {
    return;
  }

  auto checkedMalloc = [](void** ptr, size_t bytes, const char* what) -> bool {
    cudaError_t err = cudaMalloc(ptr, bytes);
    if (err != cudaSuccess) {
      std::cerr << "cudaMalloc failed for " << what << " (" << bytes << " bytes): " << cudaGetErrorString(err) << "\n";
      *ptr = nullptr;
      return false;
    }
    return true;
  };

  if (allocated_agents < count) {
    if (s_d_cell_next) {
      cudaFree(s_d_cell_next);
    }
    if (!checkedMalloc((void**)&s_d_cell_next, count * sizeof(int), "d_cell_next")) {
      allocated_agents = 0;
      return;
    }
    allocated_agents = count;
  }

  if (allocated_cells < num_cells) {
    if (s_d_cell_heads) {
      cudaFree(s_d_cell_heads);
    }
    if (!checkedMalloc((void**)&s_d_cell_heads, num_cells * sizeof(int), "d_cell_heads")) {
      allocated_cells = 0;
      return;
    }
    allocated_cells = num_cells;
  }

  cudaMemset(s_d_cell_heads, 0xFF, num_cells * sizeof(int));
  cudaMemset(s_d_cell_next, 0xFF, allocated_agents * sizeof(int));

  cudaError_t err;

  FireGrid::BuildGridKernel<<<GRID_SIZE, K_BLOCK_SIZE>>>(agents, count, grid, s_d_cell_heads, s_d_cell_next);
  err = cudaDeviceSynchronize();
  if (err != cudaSuccess) {
    printf("BuildGridKernel launch error: %s\n", cudaGetErrorString(err));
    return;
  }

  UpdateAgentsKernel<<<GRID_SIZE, K_BLOCK_SIZE>>>(agents, count, dt, f, params, grid, s_d_cell_heads, s_d_cell_next);
  err = cudaDeviceSynchronize();
  if (err != cudaSuccess) {
    printf("UpdateAgentsKernel launch error: %s\n", cudaGetErrorString(err));
  }
}
