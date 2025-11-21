#pragma once

#include <cuda_runtime.h>
#include <glm/glm.hpp>

#include "agent.h"
#include "simulation.cuh"
#include "simulation_grid.cuh"
#include "simulation_math.cuh"
#include "simulation_physics.cuh"

namespace FireSpread {

using FireGrid::GetCellIndexDevice;
using FireGrid::GridIndex;
using FireMath::Clamp01;
using FireMath::ClampedPointCount;
using FireMath::Distance3d;
using FireMath::WindData;
using FirePhysics::ComputeHeatTransfer;

// легкая быстрая версия:
__device__ void SpreadBetweenAgents_Simple(Agent* agents, int self_index, int agent_count, const FireGridParams& grid,
                                           const int* cell_heads, const int* cell_next, const WindData& wind, float dt,
                                           const FireSimulationParams& params) {
  const float FIRE_EPS = params.m_fire_intensity_min;

  Agent& self = agents[self_index];

  int sp = ClampedPointCount(self.fire_collider);
  if (sp <= 0) {
    return;
  }

  float maxI = 0.f, maxT = 0.f, avgM = 0.f;
  for (int i = 0; i < sp; ++i) {
    maxI = fmaxf(maxI, self.fire_collider.intensities[i]);
    maxT = fmaxf(maxT, self.fire_collider.temperatures[i]);
    avgM += self.fire_collider.moisture[i];
  }
  avgM /= sp;

  if (maxI <= params.m_spread_intensity_threshold) {
    return;
  }

  Float3 spc = self.position;
  float3 C{spc.x, spc.y, spc.z};

  GridIndex gi{};
  if (GetCellIndexDevice(spc.x, spc.z, grid, &gi) < 0) {
    return;
  }

  float effM = Clamp01(avgM * Clamp01(params.m_global_moisture_scale));

  constexpr int MAX_NEI = 64;

  for (int dz = -1; dz <= 1; ++dz) {
    for (int dx = -1; dx <= 1; ++dx) {
      int nx = gi.x + dx;
      int nz = gi.z + dz;
      if (nx < 0 || nx >= grid.m_grid_size_x || nz < 0 || nz >= grid.m_grid_size_z) {
        continue;
      }

      int cell_idx = (nz * grid.m_grid_size_x) + nx;

      int processed = 0;
      for (int oi = cell_heads[cell_idx]; oi != -1 && processed < MAX_NEI; oi = cell_next[oi]) {
        if (oi == self_index) {
          continue;
        }
        if (oi < 0 || oi >= agent_count) {
          break;
        }

        Agent* other = &agents[oi];
        if (other->type == FireAgentState::Burned) {
          continue;
        }

        Float3 op = other->position;
        float3 diff{op.x - C.x, op.y - C.y, op.z - C.z};

        float d2 = (diff.x * diff.x) + (diff.y * diff.y) + (diff.z * diff.z);
        if (d2 <= 1e-6F) {
          continue;
        }

        float d = sqrtf(d2);
        diff.x /= d;
        diff.y /= d;
        diff.z /= d;

        float wind_dot = (diff.x * wind.m_dir.x) + (diff.y * wind.m_dir.y) + (diff.z * wind.m_dir.z);

        float wind_sp = (wind_dot > 0.3F) ? (1.F + (wind.m_strength * 1.5F * wind_dot)) : (wind_dot < -0.3F ? 0.5F : 1.F);

        float max_d = params.m_base_spread_distance * wind_sp;
        if (d >= max_d) {
          continue;
        }

        float df = 1.f - (d / max_d);
        if (df < 0)
          df = 0;

        int opcnt = ClampedPointCount(other->fire_collider);
        if (opcnt <= 0) {
          continue;
        }

        float avgT = 0.f;
        for (int j = 0; j < opcnt; ++j)
          avgT += other->fire_collider.temperatures[j];
        avgT /= opcnt;

        float cond = other->fire_collider.conductivity > 0.F ? other->fire_collider.conductivity : params.m_base_conductivity;

        float heat = ComputeHeatTransfer(maxT, avgT, d, cond, wind_dot, effM, other->fire_collider.ignition_temp, params);

        float wb = (wind_dot > 0.1F) ? (1.F + (wind.m_strength * 2.F * wind_dot)) : (wind_dot < -0.1F ? 0.5F : 1.F);

        heat *= df * wb * Clamp01(maxI) * params.m_cross_agent_spread_strength * dt;

        if (heat > 0) {
          for (int j = 0; j < opcnt; ++j) {
            if (other->fire_collider.intensities[j] > FIRE_EPS) {
              continue;
            }
            other->fire_collider.temperatures[j] += heat;
          }
        }

        ++processed;
      }
    }
  }
}

// полная физика (медленно):
__device__ void SpreadBetweenAgents_Full(Agent* agents, int self_index, int agent_count, const FireGridParams& grid, const int* cell_heads,
                                         const int* cell_next, const WindData& wind, float dt, const FireSimulationParams& params) {
  Agent& self = agents[self_index];

  const Float3 SELF_POS = self.position;
  const float3 SELF_CENTER{SELF_POS.x, SELF_POS.y, SELF_POS.z};

  const float SPREAD_THRESHOLD = params.m_spread_intensity_threshold;
  const float FIRE_EPS = params.m_fire_intensity_min;

  GridIndex self_gi{};
  int self_cell_idx = GetCellIndexDevice(SELF_POS.x, SELF_POS.z, grid, &self_gi);
  if (self_cell_idx < 0) {
    return;
  }
  const int SELF_POINTS = ClampedPointCount(self.fire_collider);
  for (int i = 0; i < SELF_POINTS; ++i) {
    if (self.fire_collider.intensities[i] <= SPREAD_THRESHOLD) {
      continue;
    }
    const glm::vec3 POINT_POS(self.fire_collider.heat_points[i].x, self.fire_collider.heat_points[i].y,
                              self.fire_collider.heat_points[i].z);

    for (int dz = -1; dz <= 1; ++dz) {
      for (int dx = -1; dx <= 1; ++dx) {
        int nx = self_gi.x + dx;
        int nz = self_gi.z + dz;

        if (nx < 0 || nx >= grid.m_grid_size_x || nz < 0 || nz >= grid.m_grid_size_z) {
          continue;
        }

        const int CELL_IDX = nz * grid.m_grid_size_x + nx;

        for (int other_idx = cell_heads[CELL_IDX]; other_idx != -1; other_idx = cell_next[other_idx]) {
          if (other_idx < 0 || other_idx >= agent_count) {
            break;
          }
          if (other_idx == self_index) {
            continue;
          }

          Agent* other = &agents[other_idx];
          if (other->type == FireAgentState::Burned) {
            continue;
          }

          const Float3 OTHER_POS = other->position;
          const float3 OTHER_CENTER{OTHER_POS.x, OTHER_POS.y, OTHER_POS.z};

          float3 dir{OTHER_CENTER.x - SELF_CENTER.x, OTHER_CENTER.y - SELF_CENTER.y, OTHER_CENTER.z - SELF_CENTER.z};

          float len = sqrtf((dir.x * dir.x) + (dir.y * dir.y) + (dir.z * dir.z));
          if (len <= 0.001F) {
            continue;
          }
          dir.x /= len;
          dir.y /= len;
          dir.z /= len;

          float wind_dot = (dir.x * wind.m_dir.x) + (dir.y * wind.m_dir.y) + (dir.z * wind.m_dir.z);

          float wind_dist = (wind_dot > 0.3F) ? (1.F + (wind.m_strength * 2.F * wind_dot)) : (wind_dot < -0.3F ? 0.7F : 1.F);

          float max_check_dist = params.m_base_check_distance * wind_dist;
          if (len >= max_check_dist) {
            continue;
          }

          float dist_factor = 1.F - (len / max_check_dist);
          if (dist_factor < 0)
            dist_factor = 0;

          int other_points = ClampedPointCount(other->fire_collider);

          for (int j = 0; j < other_points; ++j) {
            if (other->fire_collider.intensities[j] > FIRE_EPS) {
              continue;
            }

            glm::vec3 op(other->fire_collider.heat_points[j].x, other->fire_collider.heat_points[j].y,
                         other->fire_collider.heat_points[j].z);

            float pd = Distance3d(POINT_POS.x, POINT_POS.y, POINT_POS.z, op.x, op.y, op.z);

            float wind_spread = (wind_dot > 0.3f) ? (1.f + wind.m_strength * 1.5f * wind_dot) : (wind_dot < -0.3f ? 0.5f : 1.f);

            float max_spread = params.m_base_spread_distance * wind_spread;
            if (pd >= max_spread)
              continue;

            float pd_factor = 1.f - pd / max_spread;
            if (pd_factor < 0) {
              pd_factor = 0;
            }

            float wind_bias = (wind_dot > 0.1f) ? (1.f + wind.m_strength * 2.f * wind_dot) : (wind_dot < -0.1f ? 0.5f : 1.f);

            const float TSRC = self.fire_collider.temperatures[i];
            float& Tdst = other->fire_collider.temperatures[j];

            if (TSRC <= Tdst) {
              continue;
            }
            float cond = (other->fire_collider.conductivity > 0) ? other->fire_collider.conductivity : params.m_base_conductivity;
            float effM = Clamp01(self.fire_collider.moisture[i] * Clamp01(params.m_global_moisture_scale));
            float heat = ComputeHeatTransfer(TSRC, Tdst, pd, cond, wind_dot, effM, other->fire_collider.ignition_temp, params);
            heat *= pd_factor * wind_bias * params.m_cross_agent_spread_strength * dt;

            if (heat > 0)
              Tdst += heat;
          }
        }
      }
    }
  }
}


// Улучшенная быстрая версия с сохранением физики
__device__ void SpreadBetweenAgents_ColdSpots(Agent* agents,
                                              int self_index,
                                              int agent_count,
                                              const FireGridParams& grid,
                                              const int* cell_heads,
                                              const int* cell_next,
                                              const WindData& wind,
                                              float dt,
                                              const FireSimulationParams& params)
{
  if (!cell_heads || !cell_next) {
    return;
  }

  const float fire_eps         = params.m_fire_intensity_min;
  const float SPREAD_THRESHOLD = params.m_spread_intensity_threshold;

  Agent& self = agents[self_index];
  const int self_points = ClampedPointCount(self.fire_collider);
  if (self_points <= 0) {
    return;
  }

  // ---- Агрегируем источник ----
  float self_max_I     = 0.0f;
  float self_max_T     = 0.0f;
  float self_avg_moist = 0.0f;

  int   hot_count  = 0;
  float hot_T_sum  = 0.0f;

  for (int i = 0; i < self_points; ++i) {
    const float I = self.fire_collider.intensities[i];
    const float T = self.fire_collider.temperatures[i];

    self_max_I      = fmaxf(self_max_I, I);
    self_max_T      = fmaxf(self_max_T, T);
    self_avg_moist += self.fire_collider.moisture[i];

    if (I > SPREAD_THRESHOLD) {
      ++hot_count;
      hot_T_sum += T;
    }
  }

  self_avg_moist /= self_points;

  // нет горячих точек — нечему распространяться
  if (hot_count == 0) {
    return;
  }

  const float self_hot_T = hot_T_sum / hot_count;

  const Float3 self_pos = self.position;
  GridIndex gi;
  if (GetCellIndexDevice(self_pos.x, self_pos.z, grid, &gi) < 0) {
    return;
  }

  const float global_moist   = Clamp01(params.m_global_moisture_scale);
  const float eff_moisture   = Clamp01(self_avg_moist * global_moist);

  for (int dz = -1; dz <= 1; ++dz) {
    for (int dx = -1; dx <= 1; ++dx) {
      int nx = gi.x + dx;
      int nz = gi.z + dz;

      if (nx < 0 || nx >= grid.m_grid_size_x ||
          nz < 0 || nz >= grid.m_grid_size_z) {
        continue;
      }

      const int CELL_IDX = (nz * grid.m_grid_size_x) + nx;

      for (int other_idx = cell_heads[CELL_IDX];
           other_idx != -1;
           other_idx = cell_next[other_idx]) {

        if (other_idx < 0 || other_idx >= agent_count || other_idx == self_index) {
          continue;
        }

        Agent* other = &agents[other_idx];
        if (other->type == FireAgentState::Burned) {
          continue;
        }

        const Float3 other_pos = other->position;
        float3 diff{
          other_pos.x - self_pos.x,
          other_pos.y - self_pos.y,
          other_pos.z - self_pos.z
        };

        float dist_sq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        if (dist_sq <= 1e-6f) {
          continue;
        }

        float dist = sqrtf(dist_sq);
        diff.x /= dist;
        diff.y /= dist;
        diff.z /= dist;

        const float WIND_DOT = (diff.x * wind.m_dir.x) +
                               (diff.y * wind.m_dir.y) +
                               (diff.z * wind.m_dir.z);

        float wind_spread_effect =
            (WIND_DOT > 0.3f)
                ? (1.0f + (wind.m_strength * 1.5f * WIND_DOT))
                : (WIND_DOT < -0.3f ? 0.5f : 1.0f);

        float max_spread_distance =
            params.m_base_spread_distance * wind_spread_effect;
        if (dist >= max_spread_distance) {
          continue;
        }

        float distance_factor = 1.0f - dist / max_spread_distance;
        if (distance_factor < 0.0f) {
          distance_factor = 0.0f;
        }

        const int other_points = ClampedPointCount(other->fire_collider);
        if (other_points <= 0) {
          continue;
        }

        // ищем минимальную температуру по всем точкам цели
        float minT = 1e9f;
        bool has_valid_points = false;
        for (int j = 0; j < other_points; ++j) {
          float T = other->fire_collider.temperatures[j];
          if (T < minT) {
            minT = T;
            has_valid_points = true;
          }
        }

        if (!has_valid_points) {
          continue;
        }

        const float conductivity =
            (other->fire_collider.conductivity > 0.0f)
                ? other->fire_collider.conductivity
                : params.m_base_conductivity;

        // используем среднюю по горячим точкам температуру источника
        float base_heat = ComputeHeatTransfer(
            self_hot_T,
            minT,
            dist,
            conductivity,
            WIND_DOT,
            eff_moisture,
            other->fire_collider.ignition_temp,
            params);

        if (base_heat <= 0.0f) {
          continue;
        }

        float wind_bias =
            (WIND_DOT > 0.1f)
                ? (1.0f + wind.m_strength * 3.0f * WIND_DOT)
                : (WIND_DOT < -0.1f ? 0.3f : 1.0f);

        // параметры не трогаем: intensity_factor берём как 1,
        // т.к. сила уже учтена в self_hot_T и hot_count
        const float intensity_factor = Clamp01(self_max_I);

        float heat_total = base_heat
                         * distance_factor
                         * wind_bias
                         * intensity_factor
                         * params.m_cross_agent_spread_strength
                         * dt;

        // масштаб по количеству горячих точек,
        // чтобы приблизиться к сумме по i в Full-версии
        heat_total *= static_cast<float>(hot_count);

        if (heat_total <= 0.0f) {
          continue;
        }

        const float Tref = minT + 50.0f;
        float weight_sum = 0.0f;
        float weights[64];

        // считаем веса только для НЕ горящих точек цели
        for (int j = 0; j < other_points; ++j) {
          const float Ij = other->fire_collider.intensities[j];
          const float Tj = other->fire_collider.temperatures[j];

          float w = 0.0f;

          if (Ij <= fire_eps) {
            w = Tref - Tj;
            if (w < 0.0f)
              w = 0.0f;
          }

          weights[j] = w;
          weight_sum += w;
        }

        if (weight_sum <= 1e-6f) {
          continue;
        }

        for (int j = 0; j < other_points; ++j) {
          float w = weights[j];
          if (w <= 0.0f) {
            continue;
          }

          float share = w / weight_sum;
          float dT = heat_total * share;
          other->fire_collider.temperatures[j] += dT;
        }
      }
    }
  }
}


// выбор версии распространения огня между агентами
__device__ void SpreadBetweenAgents(Agent* agents, int self_index, int agent_count, const FireGridParams& grid, const int* cell_heads,
                                    const int* cell_next, const WindData& wind, float dt, const FireSimulationParams& params) {
  // Полная физика (медленно)
  //   SpreadBetweenAgents_Full(agents, self_index, agent_count, grid, cell_heads, cell_next, wind, dt, params);

  // Быстрая версия (быстро)
  // SpreadBetweenAgents_Simple(agents, self_index, agent_count, grid, cell_heads, cell_next, wind, dt, params);

  // Быстрая версия с учётом холодных точек (средне)
  // SpreadBetweenAgents_ColdSpots(agents, self_index, agent_count, grid, cell_heads, cell_next, wind, dt, params);
    SpreadBetweenAgents_ColdSpots1(agents, self_index, agent_count, grid, cell_heads, cell_next, wind, dt, params);
}

__device__ void SpreadWithinAgent(Agent& agent, float dt, const FireSimulationParams& params) {
  const int POINT_COUNT = ClampedPointCount(agent.fire_collider);
  for (int i = 0; i < POINT_COUNT; ++i) {
    const glm::vec3 POINT_POS(agent.fire_collider.heat_points[i].x, agent.fire_collider.heat_points[i].y,
                              agent.fire_collider.heat_points[i].z);

    for (int j = 0; j < POINT_COUNT; ++j) {
      if (i == j) {
        continue;
      }

      const glm::vec3 OTHER_POINT_POS(agent.fire_collider.heat_points[j].x, agent.fire_collider.heat_points[j].y,
                                      agent.fire_collider.heat_points[j].z);

      const float POINT_DIST = Distance3d(POINT_POS.x, POINT_POS.y, POINT_POS.z, OTHER_POINT_POS.x, OTHER_POINT_POS.y, OTHER_POINT_POS.z);

      const float MAX_LOCAL_DIST = params.m_local_spread_distance;
      if (POINT_DIST >= MAX_LOCAL_DIST) {
        continue;
      }

      float distance_factor = 1.0f - POINT_DIST / MAX_LOCAL_DIST;
      if (distance_factor < 0.0f) {
        distance_factor = 0.0f;
      }

      const float Tsrc = agent.fire_collider.temperatures[i];
      float& Tdst = agent.fire_collider.temperatures[j];

      if (Tsrc <= Tdst) {
        continue;
      }

      const float CONDUCTIVITY = agent.fire_collider.conductivity > 0.0F ? agent.fire_collider.conductivity : params.m_base_conductivity;

      const float SRC_MOISTURE = agent.fire_collider.moisture[i];
      const float GLOBAL_MOIST = Clamp01(params.m_global_moisture_scale);
      const float EFF_MOISTURE = Clamp01(SRC_MOISTURE * GLOBAL_MOIST);

      float heat = ComputeHeatTransfer(Tsrc, Tdst, POINT_DIST, CONDUCTIVITY,
                                       /*wind_factor=*/0.0F,  // внутри агента ветер не учитываем
                                       EFF_MOISTURE,
                                       agent.fire_collider.ignition_temp,  // порог этого же материала
                                       params);

      heat *= distance_factor * params.m_local_spread_strength * dt;

      if (heat > 0.0f) {
        Tdst += heat;
      }
    }
  }
}

}  // namespace FireSpread
