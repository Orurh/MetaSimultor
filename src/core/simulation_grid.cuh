#pragma once

#include <cuda_runtime.h>
#include "agent.h"
#include "simulation.cuh"

// Пространственная сетка для ускорения поиска соседних агентов.
namespace FireGrid {

struct GridIndex {
  int x;
  int z;
};

// Вычисляет индекс ячейки сетки для заданных координат (x, z).
// Возвращает -1, если координаты выходят за пределы сетки.
__device__ int GetCellIndexDevice(float x, float z, const FireGridParams& grid, GridIndex* out_gi) {
  int cell_x = static_cast<int>((x - grid.m_world_min_x) / grid.m_cell_size);
  int cell_z = static_cast<int>((z - grid.m_world_min_z) / grid.m_cell_size);

  if (cell_x < 0 || cell_x >= grid.m_grid_size_x || cell_z < 0 || cell_z >= grid.m_grid_size_z) {
    if (out_gi) {
      out_gi->x = -1;
      out_gi->z = -1;
    }
    return -1;
  }

  if (out_gi) {
    out_gi->x = cell_x;
    out_gi->z = cell_z;
  }

  return (cell_z * grid.m_grid_size_x) + cell_x;
}

// CUDA kernel для построения пространственной сетки из агентов.
__global__ void BuildGridKernel(const Agent* agents, int count, FireGridParams grid, int* cell_heads, int* cell_next) {
  const int idx = threadIdx.x + blockIdx.x * blockDim.x;
  if (idx < 0 || idx >= count) {
    return;
  }

  if (!agents || !cell_heads || !cell_next) {
    return;
  }

  const Float3 pos = agents[idx].position;

  GridIndex gi{};
  int cell_idx = GetCellIndexDevice(pos.x, pos.z, grid, &gi);

  if (cell_idx < 0) {
    cell_next[idx] = -1;
    return;
  }

  const int max_cells = grid.m_grid_size_x * grid.m_grid_size_z;
  if (cell_idx < 0 || cell_idx >= max_cells) {
    cell_next[idx] = -1;
    return;
  }
  int old_head = atomicExch(&cell_heads[cell_idx], idx);
  cell_next[idx] = old_head;
}

}  // namespace FireGrid
