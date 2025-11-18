#include "agent.h"
#include "simulation.cuh"

#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#include <cmath>
#include <iostream>

namespace {

  struct GridIndex {
  int x;
  int z;
};

__device__ int GetCellIndexDevice(float x, float z, const FireGridParams& grid, GridIndex* out_gi) {
  int cell_x = static_cast<int>((x - grid.world_min_x) / grid.cell_size);
  int cell_z = static_cast<int>((z - grid.world_min_z) / grid.cell_size);

  // за пределами мира – вываливаемся
  if (cell_x < 0 || cell_x >= grid.grid_size_x ||
      cell_z < 0 || cell_z >= grid.grid_size_z) {
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

  return (cell_z * grid.grid_size_x) + cell_x; // row-major
}


__global__ void BuildGridKernel(const Agent* agents, int count,
                                const FireGridParams grid,
                                int* cell_heads, int* cell_next) {
  int idx = threadIdx.x + blockIdx.x * blockDim.x;
  if (idx >= count) return;

  const Float3 pos = agents[idx].position;

  GridIndex gi;
  int cell_idx = GetCellIndexDevice(pos.x, pos.z, grid, &gi);
  if (cell_idx < 0) {
    cell_next[idx] = -1;
    return;
  }

  // Вставляем агента в голову списка ячейки через atomicExch
  int old_head = atomicExch(&cell_heads[cell_idx], idx);
  cell_next[idx] = old_head; // -1, если в ячейке никого не было
}

// -----------------------------------------------------------------------------
// Small math helpers
// -----------------------------------------------------------------------------

__device__ float Clamp01(float x) {
  return fminf(1.0F, fmaxf(0.0F, x));
}

__device__ float Distance3d(float x1, float y1, float z1, float x2, float y2, float z2) {
  const float dx = x1 - x2;
  const float dy = y1 - y2;
  const float dz = z1 - z2;
  return sqrtf((dx * dx) + (dy * dy) + (dz * dz));
}

__device__ int ClampedPointCount(const FireCollider& c) {
  int n = c.point_count;
  if (n < 0)  n = 0;
  if (n > 64) n = 64;
  return n;
}

struct WindData {
  float3 m_dir;      // нормализованное направление
  float m_strength;  // длина исходного вектора
};

__device__ WindData MakeWindData(const float3& wind_force) {
  WindData w{};
  w.m_strength = sqrtf((wind_force.x * wind_force.x) + (wind_force.y * wind_force.y) +
                       (wind_force.z * wind_force.z));

  if (w.m_strength > 0.001F) {
    w.m_dir.x = wind_force.x / w.m_strength;
    w.m_dir.y = wind_force.y / w.m_strength;
    w.m_dir.z = wind_force.z / w.m_strength;
  } else {
    w.m_dir.x = w.m_dir.y = w.m_dir.z = 0.0F;
  }

  return w;
}

// -----------------------------------------------------------------------------
// Теплопередача и воспламенение
// -----------------------------------------------------------------------------

// Базовый расчёт теплопередачи между точками
__device__ float ComputeHeatTransfer(float Tsrc, float Tdst, float distance, float conductivity,
                                     float wind_factor,  // -1..1
                                     float moisture,     // 0..1, локальная влажность источника
                                     float ignition_temp, const FireSimulationParams& p) {
  if (distance < 0.1f) {
    distance = 0.1f;
  }

  // Простая модель: проводимость * градиент температуры / r^2
  float base = conductivity * (Tsrc - Tdst) / (distance * distance);

  // Ветер: усиливаем вдоль ветра, чуть гасим против
  float wind_mul = 1.0F;
  if (wind_factor > 0.0F) {
    wind_mul += wind_factor * p.m_wind_heat_bonus;
  } else if (wind_factor < 0.0F) {
    wind_mul += wind_factor * 0.3F;  // чуть ослабляем против ветра
  }

  float heat = base * wind_mul;

  // Локальная влажность гасит теплопередачу
  const float m = Clamp01(moisture);  // 0..1
  heat *= (1.0F - m * p.m_humidity_heat_dampen);

  // Глобальная влажность воздуха тоже немного гасит
  const float AIR = Clamp01(p.m_air_humidity_factor);  // 0..1
  heat *= (1.0F - AIR);

  // --- NEW: мягко усиливаем начальный нагрев для холодных материалов ---
  // ignition_temp > 0 — нормальная ситуация; если вдруг нет, не трогаем.
  if (ignition_temp > 0.0f && heat > 0.0f) {
    const float half_ign = 0.5f * ignition_temp;

    if (Tdst < ignition_temp) {
      // Нормируем на 0..1 в диапазоне [0 .. ignition_temp]
      float norm = Clamp01(Tdst / ignition_temp);  // 0 при очень холодном, 1 около порога

      // Чем холоднее материал, тем больше буст (до m_initial_spread_boost)
      float boost = 1.0f + (1.0f - norm) * (p.m_initial_spread_boost - 1.0f);

      // И немного штрафуем совсем холодные материалы (чтобы не улететь)
      float penalty = p.m_cold_material_penalty + (1.0f - p.m_cold_material_penalty) * norm;

      heat *= (boost * penalty);
    }
  }

  return heat;
}

// Проверка воспламенения точки по температуре
__device__ void ApplyIgnition(float& intensity, float T, float fuel, float moisture,
                              float ignition_temp, const FireSimulationParams& p, float dt) {
  if (fuel <= 0.0F) {
    return;
  }

  const float M = Clamp01(moisture);  // 0..1

  // Чем выше влажность, тем выше эффективный порог воспламенения
  const float EFF_IGN = ignition_temp * (1.0F + M * p.m_humidity_ignition_factor);

  if (T > EFF_IGN) {
    const float EXCESS = T - EFF_IGN;
    float k = EXCESS / EFF_IGN;  // относительный "запас" температуры
    k = Clamp01(k);

    const float TARGET_INTENSITY = 0.2F + (0.8F * k);

    // скорость "подъёма" к целевой интенсивности
    const float ignite_rate = 3.0f;  // можно потом вынести в FireSimulationParams

    float delta = TARGET_INTENSITY - intensity;
    if (delta > 0.0f) {
      // ТЕПЕРЬ используем настоящий dt, а не захардкоженный 0.016
      intensity += delta * Clamp01(ignite_rate * dt);
    }

    if (intensity < p.m_fire_intensity_min) {
      intensity = p.m_fire_intensity_min;
    }
  }
}

// -----------------------------------------------------------------------------
// Обновление одной "деревяшки"
// -----------------------------------------------------------------------------

// Шаг горения внутри агента (расход топлива, рост/затухание интенсивности, нагрев/остывание)
__device__ void UpdateBurningPoints(Agent& agent, const WindData& wind, float dt,
                                    const FireSimulationParams& params) {
  const float fire_eps = params.m_fire_intensity_min;
  const float max_I = params.m_max_intensity;

  // --- максимум температуры для этого агента ---
  // если у коллайдера задан свой max_temperature > 0, берём его,
  // иначе используем глобальный из параметров
  const float base_max_T = (agent.fire_collider.max_temperature > 0.0f)
                               ? agent.fire_collider.max_temperature
                               : params.m_max_temperature;

  // --- поправка от силы ветра ---
  // wind.m_strength примерно в м/с, нормируем на "10" как условный максимум
  const float wind_boost = 1.0f + 0.2f * Clamp01(wind.m_strength / 10.0f);

  const float local_max_T = base_max_T * wind_boost;

  const int point_count = ClampedPointCount(agent.fire_collider);
  for (int i = 0; i < point_count; ++i) {
    float& I = agent.fire_collider.intensities[i];
    float& F = agent.fire_collider.fuels[i];
    float& T = agent.fire_collider.temperatures[i];

    if (I > fire_eps) {
      const float wind_burn = 1.0f + wind.m_strength * 0.5f;
      const float wind_I = 1.0f + wind.m_strength * 0.3f;

      F -= I * dt * params.m_burn_fuel_rate * wind_burn;
      I += dt * params.m_intensity_growth_rate * wind_I;
      if (I > max_I)
        I = max_I;

      T += I * params.m_heat_production * dt;

      // ★ ограничиваем температуру сверху с учётом материала и ветра
      if (T > local_max_T) {
        T = local_max_T;
      }

      if (F <= 0.0f) {
        F = 0.0f;
        I = 0.0f;
      }
    } else {
      const float ambient = params.m_ambient_temperature;
      T -= params.m_cooling_rate * (T - ambient) * dt;
      if (T < ambient)
        T = ambient;
    }
  }
}

// Распространение огня между агентами с учётом ветра (по позициям, а не индексам)
__device__ void SpreadBetweenAgents(Agent* agents,
                                    int self_index,
                                    int agent_count,
                                    const FireGridParams& grid,
                                    const int* cell_heads,
                                    const int* cell_next,
                                    const WindData& wind,
                                    float dt,
                                    const FireSimulationParams& params) {
  Agent& self = agents[self_index];

  const Float3 self_pos = self.position;
  const float3 self_center{self_pos.x, self_pos.y, self_pos.z};

  const float spread_threshold = params.m_spread_intensity_threshold;
  const float fire_eps         = params.m_fire_intensity_min;

  // определяем ячейку текущего агента
  GridIndex self_gi;
  int self_cell_idx = GetCellIndexDevice(self_pos.x, self_pos.z, grid, &self_gi);
  if (self_cell_idx < 0) {
    return;
  }

  const int self_points = ClampedPointCount(self.fire_collider);
  for (int i = 0; i < self_points; ++i) {
    if (self.fire_collider.intensities[i] <= spread_threshold) {
      continue;
    }

    const glm::vec3 point_pos(self.fire_collider.heat_points[i].x,
                              self.fire_collider.heat_points[i].y,
                              self.fire_collider.heat_points[i].z);

    // перебираем соседние ячейки (3x3 по XZ)
    for (int dz = -1; dz <= 1; ++dz) {
      for (int dx = -1; dx <= 1; ++dx) {
        int nx = self_gi.x + dx;
        int nz = self_gi.z + dz;
        if (nx < 0 || nx >= grid.grid_size_x || nz < 0 || nz >= grid.grid_size_z) {
          continue;
        }

        int cell_idx = nz * grid.grid_size_x + nx;

        // пробегаем связный список агентов в этой ячейке
        for (int other_idx = cell_heads[cell_idx]; other_idx != -1; other_idx = cell_next[other_idx]) {
          if (other_idx < 0 || other_idx >= agent_count) {
            break;
          }
          if (other_idx == self_index) continue;

          Agent* other = &agents[other_idx];
          if (other->type == FireAgentState::Burned) {
            continue;
          }

          const Float3 other_pos = other->position;
          const float3 other_center{other_pos.x, other_pos.y, other_pos.z};

          float3 to_other_dir{other_center.x - self_center.x,
                              other_center.y - self_center.y,
                              other_center.z - self_center.z};

          float to_other_length =
              sqrtf(to_other_dir.x * to_other_dir.x +
                    to_other_dir.y * to_other_dir.y +
                    to_other_dir.z * to_other_dir.z);

          if (to_other_length <= 0.001f) {
            continue;
          }

          // Нормализуем
          to_other_dir.x /= to_other_length;
          to_other_dir.y /= to_other_length;
          to_other_dir.z /= to_other_length;

          const float base_check_distance = params.m_base_check_distance;

          const float wind_dot =
              to_other_dir.x * wind.m_dir.x +
              to_other_dir.y * wind.m_dir.y +
              to_other_dir.z * wind.m_dir.z;

          float wind_distance_effect = 1.0f;
          if (wind_dot > 0.3f) {
            wind_distance_effect = 1.0f + wind.m_strength * 2.0f * wind_dot;
          } else if (wind_dot < -0.3f) {
            wind_distance_effect = 0.7f;
          }

          const float max_check_distance = base_check_distance * wind_distance_effect;
          const float box_dist = to_other_length;

          if (box_dist >= max_check_distance) {
            continue;
          }

          float agent_distance_factor = 1.0f - (box_dist / max_check_distance);
          if (agent_distance_factor < 0.0f) agent_distance_factor = 0.0f;

          // --- перебор точек другого агента ---
          const int other_points = ClampedPointCount(other->fire_collider);
          for (int j = 0; j < other_points; ++j) {
            if (other->fire_collider.intensities[j] > fire_eps) {
              continue;
            }

            const glm::vec3 other_point_pos(other->fire_collider.heat_points[j].x,
                                            other->fire_collider.heat_points[j].y,
                                            other->fire_collider.heat_points[j].z);

            const float point_dist =
                Distance3d(point_pos.x, point_pos.y, point_pos.z,
                           other_point_pos.x, other_point_pos.y, other_point_pos.z);

            float wind_spread_effect = 1.0f;
            if (wind_dot > 0.3f) {
              wind_spread_effect = 1.0f + wind.m_strength * 1.5f * wind_dot;
            } else if (wind_dot < -0.3f) {
              wind_spread_effect = 0.5f;
            }

            const float max_spread_distance = params.m_base_spread_distance * wind_spread_effect;

            if (point_dist >= max_spread_distance) {
              continue;
            }

            float distance_factor = 1.0f - point_dist / max_spread_distance;
            if (distance_factor < 0.0f) distance_factor = 0.0f;

            float wind_spread_bias = 1.0f;
            if (wind_dot > 0.1f) {
              wind_spread_bias = 1.0f + wind.m_strength * 2.0f * wind_dot;
            } else if (wind_dot < -0.1f) {
              wind_spread_bias = 0.5f;
            }

            const float Tsrc = self.fire_collider.temperatures[i];
            float&      Tdst = other->fire_collider.temperatures[j];

            if (Tsrc <= Tdst) {
              continue;
            }

            const float conductivity =
                other->fire_collider.conductivity > 0.0f
                    ? other->fire_collider.conductivity
                    : params.m_base_conductivity;

            const float src_moisture = self.fire_collider.moisture[i];
            const float global_moist = Clamp01(params.m_global_moisture_scale);
            const float eff_moisture = Clamp01(src_moisture * global_moist);

            float heat = ComputeHeatTransfer(
                Tsrc, Tdst, point_dist, conductivity,
                wind_dot,
                eff_moisture,
                other->fire_collider.ignition_temp,
                params);

            heat *= distance_factor * wind_spread_bias *
                    params.m_cross_agent_spread_strength * dt;

            if (heat > 0.0f) {
              Tdst += heat;
            }
          } // for each point j
        }   // for each agent in cell
      }     // dx
    }       // dz
  }         // for each source point i
}

// Теплопередача внутри одного агента (крона/ствол)
__device__ void SpreadWithinAgent(Agent& agent, float dt, const FireSimulationParams& params) {
  const int point_count = ClampedPointCount(agent.fire_collider);
  for (int i = 0; i < point_count; ++i) {
    const glm::vec3 point_pos(agent.fire_collider.heat_points[i].x,
                              agent.fire_collider.heat_points[i].y,
                              agent.fire_collider.heat_points[i].z);

    for (int j = 0; j < point_count; ++j) {
      if (i == j) {
        continue;
      }

      const glm::vec3 other_point_pos(agent.fire_collider.heat_points[j].x,
                                      agent.fire_collider.heat_points[j].y,
                                      agent.fire_collider.heat_points[j].z);

      const float point_dist = Distance3d(point_pos.x, point_pos.y, point_pos.z, other_point_pos.x,
                                          other_point_pos.y, other_point_pos.z);

      const float max_local_dist = params.m_local_spread_distance;
      if (point_dist >= max_local_dist) {
        continue;
      }

      float distance_factor = 1.0f - point_dist / max_local_dist;
      if (distance_factor < 0.0f) {
        distance_factor = 0.0f;
      }

      const float Tsrc = agent.fire_collider.temperatures[i];
      float& Tdst = agent.fire_collider.temperatures[j];

      if (Tsrc <= Tdst) {
        continue;
      }

      const float conductivity = agent.fire_collider.conductivity > 0.0f
                                     ? agent.fire_collider.conductivity
                                     : params.m_base_conductivity;

      const float src_moisture = agent.fire_collider.moisture[i];
      const float global_moist = Clamp01(params.m_global_moisture_scale);
      const float eff_moisture = Clamp01(src_moisture * global_moist);

      float heat =
          ComputeHeatTransfer(Tsrc, Tdst, point_dist, conductivity,
                              /*wind_factor=*/0.0f,  // внутри агента ветер не учитываем
                              eff_moisture,
                              agent.fire_collider.ignition_temp,  // порог этого же материала
                              params);

      heat *= distance_factor * params.m_local_spread_strength * dt;

      if (heat > 0.0f) {
        Tdst += heat;
      }
    }
  }
}

// Обновление типа агента на основе состояния точек
__device__ void UpdateAgentType(Agent& agent, const FireSimulationParams& params) {
  bool has_fire = false;
  bool has_fuel = false;

  const int point_count = ClampedPointCount(agent.fire_collider);
  for (int i = 0; i < point_count; ++i) {
    if (agent.fire_collider.intensities[i] > params.m_fire_intensity_min) {
      has_fire = true;
    }
    if (agent.fire_collider.fuels[i] > params.m_fuel_epsilon) {
      has_fuel = true;
    }
  }

  if (has_fire) {
    // Как только где-то загорелось — помечаем агент как горящий
    agent.type = FireAgentState::Burning;
  } else if (!has_fire && !has_fuel) {
    // Топлива нет и нигде не горит — всё, агент сгорел окончательно
    agent.type = FireAgentState::Burned;
  }
  // Иначе:
  //  - есть топливо, но прямо сейчас вспышек нет → оставляем type как был
  //    (NORMAL, если ещё не загорался; или BURNING, если уже горел)
}

}  // namespace

// охлаждаем головешки
__device__ void CoolBurnedAgent(Agent& agent, float dt, const FireSimulationParams& params) {
  const float ambient = params.m_ambient_temperature;

  const int point_count = ClampedPointCount(agent.fire_collider);
  for (int i = 0; i < point_count; ++i) {
    float& T = agent.fire_collider.temperatures[i];

    // экспоненциальное охлаждение к окружающей
    T -= params.m_cooling_rate * (T - ambient) * dt;
    if (T < ambient) {
      T = ambient;
    }

    // на всякий случай гасим всё
    agent.fire_collider.intensities[i] = 0.0f;
    agent.fire_collider.fuels[i] = 0.0f;
  }
}


__device__ void IgnitePoints(Agent& agent, float dt, const FireSimulationParams& params) {
  const float fire_eps = params.m_fire_intensity_min;

  const int point_count = ClampedPointCount(agent.fire_collider);
  for (int i = 0; i < point_count; ++i) {
    float& I = agent.fire_collider.intensities[i];
    float& F = agent.fire_collider.fuels[i];
    float& T = agent.fire_collider.temperatures[i];
    float  M = agent.fire_collider.moisture[i];
    const float global_moist = Clamp01(params.m_global_moisture_scale);
    M = Clamp01(M * global_moist);

    // уже горит или топлива нет — ignition не нужен
    if (I > fire_eps || F <= 0.0f) {
      continue;
    }

    ApplyIgnition(I, T, F, M, agent.fire_collider.ignition_temp, params, dt);
  }
}

// -----------------------------------------------------------------------------
// CUDA kernel
// -----------------------------------------------------------------------------

__global__ void UpdateAgentsKernel(Agent* agents,
                                   int count,
                                   float dt,
                                   float3 wind_force,
                                   FireSimulationParams params,
                                   FireGridParams grid,
                                   const int* cell_heads,
                                   const int* cell_next) {
  const int IDX = threadIdx.x + (blockIdx.x * blockDim.x);
  if (IDX >= count) {
    return;
  }

  Agent& agent = agents[IDX];
  if (agent.type == FireAgentState::Burned) {
    CoolBurnedAgent(agent, dt, params);
    return;
  }

  const WindData WIND = MakeWindData(wind_force);

  // 1. Обновляем горение с учетом ветра и температуры
  UpdateBurningPoints(agent, WIND, dt, params);

  // 2. Распространение огня между агентами (через грид)
  SpreadBetweenAgents(agents, IDX, count, grid, cell_heads, cell_next, WIND, dt, params);

  // 3. Теплопередача внутри одного агента (крона/ствол)
  SpreadWithinAgent(agent, dt, params);

  // 3.5. ОТДЕЛЬНЫЙ шаг воспламенения точек на основе финальной температуры
  IgnitePoints(agent, dt, params);

  // 4. Обновляем тип агента
  UpdateAgentType(agent, params);
}


// -----------------------------------------------------------------------------
// Host API
// -----------------------------------------------------------------------------

void SimulationStep(Agent* agents, int count, float dt,
                    const glm::vec3& wind_force,
                    const FireSimulationParams& params,
                    const FireGridParams& grid) {
  constexpr int K_BLOCK_SIZE = 256;
  const int GRID_SIZE = (count + K_BLOCK_SIZE - 1) / K_BLOCK_SIZE;

  const float3 f{wind_force.x, wind_force.y, wind_force.z};

  // ---------------------------
  // 1) Выделяем/переиспользуем память под grid
  // ---------------------------
  static int* d_cell_heads = nullptr;
  static int* d_cell_next  = nullptr;
  static int  allocated_agents = 0;
  static int  allocated_cells  = 0;

  const int num_cells = grid.grid_size_x * grid.grid_size_z;

  if (allocated_agents < count) {
    if (d_cell_next) cudaFree(d_cell_next);
    cudaMalloc(&d_cell_next, count * sizeof(int));
    allocated_agents = count;
  }

  if (allocated_cells < num_cells) {
    if (d_cell_heads) cudaFree(d_cell_heads);
    cudaMalloc(&d_cell_heads, num_cells * sizeof(int));
    allocated_cells = num_cells;
  }

  // 2) Заполняем cell_heads = -1
  cudaMemset(d_cell_heads, 0xFF, num_cells * sizeof(int)); // 0xFF -> -1 для int

  // 3) Строим grid на GPU
  BuildGridKernel<<<GRID_SIZE, K_BLOCK_SIZE>>>(agents, count, grid, d_cell_heads, d_cell_next);

  // 4) Основной шаг симуляции, уже пользуясь grid
  UpdateAgentsKernel<<<GRID_SIZE, K_BLOCK_SIZE>>>(
      agents, count, dt, f, params, grid, d_cell_heads, d_cell_next);

  cudaDeviceSynchronize();

  const cudaError_t ERR = cudaGetLastError();
  if (ERR != cudaSuccess) {
    printf("CUDA error: %s\n", cudaGetErrorString(ERR));
  }
}
