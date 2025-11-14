#include "simulation.cuh"
#include <cmath>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <glm/glm.hpp>
#include <iostream>
const int MAX_NEIGHBORS = 50;

__device__ float clamp01(float x) { return fminf(1.0f, fmaxf(0.0f, x)); }

__device__ float distance3d(float x1, float y1, float z1, float x2, float y2,
                            float z2) {
  float dx = x1 - x2;
  float dy = y1 - y2;
  float dz = z1 - z2;
  return sqrtf((dx * dx) + (dy * dy) + (dz * dz));
}

// Базовый расчёт теплопередачи между точками
__device__ float compute_heat_transfer(float Tsrc, float Tdst, float distance,
                                       float conductivity,
                                       float wind_factor, // -1..1
                                       const FireSimulationParams &p) {
  if (distance < 0.1f)
    distance = 0.1f;

  // Простая модель: проводимость * градиент температуры / r^2
  float base = conductivity * (Tsrc - Tdst) / (distance * distance);

  // Ветер: усиливаем вдоль ветра, чуть гасим против
  float wind_mul = 1.0f;
  if (wind_factor > 0.0f) {
    wind_mul += wind_factor * p.m_wind_heat_bonus;
  } else if (wind_factor < 0.0f) {
    wind_mul += wind_factor * 0.3f; // чуть ослабляем против ветра
  }

  return base * wind_mul;
}

// Проверка воспламенения точки по температуре
__device__ void apply_ignition(float &intensity, float T, float fuel,
                               float moisture, float ignition_temp,
                               const FireSimulationParams &p) {
  // пока влагу не учитываем, но оставляем сигнатуру
  (void)moisture;

  if (fuel <= 0.0f)
    return;

  if (T > ignition_temp) {
    float excess = T - ignition_temp;
    float k = excess / ignition_temp; // относительный "запас" температуры
    k = clamp01(k);

    // Делаем минимум детерминированного поджога, как было 0.2–0.3
    float target_intensity = 0.2f + 0.8f * k;
    if (intensity < target_intensity) {
      intensity = target_intensity;
    }
  }
}

__global__ void update_agents_kernel(Agent *agents, int count, float dt,
                                     float3 wind_force,
                                     FireSimulationParams params) {
  int idx = threadIdx.x + (blockIdx.x * blockDim.x);
  if (idx >= count)
    return;

  Agent *agent = &agents[idx];
  if (agent->type == BURNED) {
    return;
  }

  // Нормализуем вектор ветра и вычисляем его силу
  float wind_strength =
      sqrtf(wind_force.x * wind_force.x + wind_force.y * wind_force.y +
            wind_force.z * wind_force.z);
  float3 wind_dir = {0, 0, 0};
  if (wind_strength > 0.001f) {
    wind_dir = {.x = wind_force.x / wind_strength,
                .y = wind_force.y / wind_strength,
                .z = wind_force.z / wind_strength};
  }

  // 1. Обновляем горение с учетом ветра и температуры
  for (int i = 0; i < agent->fire_collider.point_count; i++) {
    float &intensity = agent->fire_collider.intensities[i];
    float &fuel = agent->fire_collider.fuels[i];
    float &temperature = agent->fire_collider.temperatures[i];

    if (intensity > 0.1f) {
      float wind_burn_effect = 1.0f + wind_strength * 0.5f;
      fuel -= intensity * dt * 5.0f * wind_burn_effect;

      float wind_intensity_effect = 1.0f + wind_strength * 0.3f;
      intensity += dt * 2.0f * wind_intensity_effect;
      if (intensity > 1.0f) {
        intensity = 1.0f;
      }

      // Горящая точка нагревается
      temperature += intensity * 300.0f * dt; // 300 — пока просто масштаб

      if (fuel <= 0.0f) {
        intensity = 0.0f;
        fuel = 0.0f;
      }
    } else {
      // Негорящая точка потихоньку остывает к "окружающей"
      const float kAmbient = 293.15f; // ~20°C
      temperature -= 10.0f * dt;
      if (temperature < kAmbient) {
        temperature = kAmbient;
      }
    }
  }

  // 2. Распространение огня между агентами с ПРАВИЛЬНЫМ учетом ветра
  for (int i = 0; i < agent->fire_collider.point_count; i++) {
    if (agent->fire_collider.intensities[i] > 0.3f) {
      glm::vec3 point_pos = glm::vec3(agent->fire_collider.heat_points[i].x,
                                      agent->fire_collider.heat_points[i].y,
                                      agent->fire_collider.heat_points[i].z);

      for (int n = 0; n < MAX_NEIGHBORS; n++) {
        int other_idx = (idx + n * 37) % count;
        if (other_idx == idx)
          continue;

        Agent *other_agent = &agents[other_idx];

        // БАЗОВАЯ дистанция проверки (без влияния ветра)
        float base_check_distance = 15.0f;

        // Вычисляем направление к другому агенту
        float3 agent_center = {(agent->fire_collider.bounds_min.x +
                                agent->fire_collider.bounds_max.x) *
                                   0.5f,
                               (agent->fire_collider.bounds_min.y +
                                agent->fire_collider.bounds_max.y) *
                                   0.5f,
                               (agent->fire_collider.bounds_min.z +
                                agent->fire_collider.bounds_max.z) *
                                   0.5f};

        float3 other_center = {(other_agent->fire_collider.bounds_min.x +
                                other_agent->fire_collider.bounds_max.x) *
                                   0.5f,
                               (other_agent->fire_collider.bounds_min.y +
                                other_agent->fire_collider.bounds_max.y) *
                                   0.5f,
                               (other_agent->fire_collider.bounds_min.z +
                                other_agent->fire_collider.bounds_max.z) *
                                   0.5f};

        float3 to_other_dir = {other_center.x - agent_center.x,
                               other_center.y - agent_center.y,
                               other_center.z - agent_center.z};
        float to_other_length = sqrtf(to_other_dir.x * to_other_dir.x +
                                      to_other_dir.y * to_other_dir.y +
                                      to_other_dir.z * to_other_dir.z);

        if (to_other_length > 0.001f) {
          to_other_dir.x /= to_other_length;
          to_other_dir.y /= to_other_length;
          to_other_dir.z /= to_other_length;

          // Влияние ветра на дистанцию проверки
          float wind_dot = to_other_dir.x * wind_dir.x +
                           to_other_dir.y * wind_dir.y +
                           to_other_dir.z * wind_dir.z;

          // Увеличиваем дистанцию ТОЛЬКО по направлению ветра
          // и немного уменьшаем против ветра
          float wind_distance_effect = 1.0f;
          if (wind_dot > 0.3f) {
            // По ветру: сильно увеличиваем
            wind_distance_effect = 1.0f + wind_strength * 2.0f * wind_dot;
          } else if (wind_dot < -0.3f) {
            // Против ветра: немного уменьшаем
            wind_distance_effect = 0.7f;
          }

          float max_check_distance = base_check_distance * wind_distance_effect;
          float box_dist = to_other_length;

          if (box_dist < max_check_distance) {
            for (int j = 0; j < other_agent->fire_collider.point_count; j++) {
              if (other_agent->fire_collider.intensities[j] > 0.1f)
                continue;

              glm::vec3 other_point_pos =
                  glm::vec3(other_agent->fire_collider.heat_points[j].x,
                            other_agent->fire_collider.heat_points[j].y,
                            other_agent->fire_collider.heat_points[j].z);

              float point_dist = distance3d(
                  point_pos.x, point_pos.y, point_pos.z, other_point_pos.x,
                  other_point_pos.y, other_point_pos.z);

              // Базовое расстояние распространения
              float base_spread_distance = 3.0f;

              // Влияние ветра на расстояние распространения
              float wind_spread_effect = 1.0f;
              if (wind_dot > 0.3f) {
                wind_spread_effect = 1.0f + wind_strength * 1.5f * wind_dot;
              } else if (wind_dot < -0.3f) {
                wind_spread_effect = 0.5f; // Сильно уменьшаем против ветра
              }

              float max_spread_distance =
                  params.m_base_spread_distance * wind_spread_effect;

              if (point_dist < max_spread_distance) {
                float distance_factor = 1.0f - point_dist / max_spread_distance;
                if (distance_factor < 0.0f)
                  distance_factor = 0.0f;

                // Влияние ветра на силу распространения
                float wind_spread_bias = 1.0f;
                if (wind_dot > 0.1f) {
                  wind_spread_bias = 1.0f + wind_strength * 2.0f * wind_dot;
                } else if (wind_dot < -0.1f) {
                  wind_spread_bias = 0.5f; // сильно гасим против ветра
                }

                // Берём температуры как источник/приёмник
                float Tsrc = agent->fire_collider.temperatures[i];
                float Tdst = other_agent->fire_collider.temperatures[j];

                // Не гоним тепло от более холодной точки
                if (Tsrc > Tdst) {
                  float conductivity =
                      other_agent->fire_collider.conductivity > 0.0f
                          ? other_agent->fire_collider.conductivity
                          : params.m_base_conductivity;

                  float heat = compute_heat_transfer(
                      Tsrc, Tdst, point_dist, conductivity, wind_dot, params);

                  // Модифицируем по дистанции/ветру/шагу времени
                  heat *= distance_factor * wind_spread_bias * dt;

                  if (heat > 0.0f) {
                    other_agent->fire_collider.temperatures[j] += heat;

                    // Проверяем воспламенение точки по температуре
                    apply_ignition(other_agent->fire_collider.intensities[j],
                                   other_agent->fire_collider.temperatures[j],
                                   other_agent->fire_collider.fuels[j],
                                   other_agent->fire_collider.moisture[j],
                                   other_agent->fire_collider.ignition_temp,
                                   params);
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  // 3. Теплопередача внутри одного агента (крона/ствол)
  for (int i = 0; i < agent->fire_collider.point_count; i++) {
    glm::vec3 point_pos = glm::vec3(agent->fire_collider.heat_points[i].x,
                                    agent->fire_collider.heat_points[i].y,
                                    agent->fire_collider.heat_points[i].z);

    for (int j = 0; j < agent->fire_collider.point_count; j++) {
      if (i == j)
        continue;

      glm::vec3 other_point_pos =
          glm::vec3(agent->fire_collider.heat_points[j].x,
                    agent->fire_collider.heat_points[j].y,
                    agent->fire_collider.heat_points[j].z);

      float point_dist =
          distance3d(point_pos.x, point_pos.y, point_pos.z, other_point_pos.x,
                     other_point_pos.y, other_point_pos.z);

      float max_local_dist = params.m_local_spread_distance;

      if (point_dist < max_local_dist) {
        float distance_factor = 1.0f - point_dist / max_local_dist;
        if (distance_factor < 0.0f)
          distance_factor = 0.0f;

        float Tsrc = agent->fire_collider.temperatures[i];
        float Tdst = agent->fire_collider.temperatures[j];

        if (Tsrc > Tdst) {
          float conductivity = agent->fire_collider.conductivity > 0.0f
                                   ? agent->fire_collider.conductivity
                                   : params.m_base_conductivity;

          float heat =
              compute_heat_transfer(Tsrc, Tdst, point_dist, conductivity,
                                    /*wind_factor=*/0.0f, params);

          heat *= distance_factor * params.m_local_spread_strength * dt;

          if (heat > 0.0f) {
            agent->fire_collider.temperatures[j] += heat;

            apply_ignition(agent->fire_collider.intensities[j],
                           agent->fire_collider.temperatures[j],
                           agent->fire_collider.fuels[j],
                           agent->fire_collider.moisture[j],
                           agent->fire_collider.ignition_temp, params);
          }
        }
      }
    }
  }

  // 4. Обновляем тип агента на основе точек горения и топлива
  bool has_fire = false;
  bool has_fuel = false;

  for (int i = 0; i < agent->fire_collider.point_count; i++) {
    if (agent->fire_collider.intensities[i] > 0.1f) {
      has_fire = true;
    }
    if (agent->fire_collider.fuels[i] > 0.1f) {
      has_fuel = true;
    }
  }

  if (has_fire) {
    // Как только где-то загорелось — помечаем агент как горящий
    agent->type = BURNING;
  } else if (!has_fire && !has_fuel) {
    // Топлива нет и нигде не горит — всё, агент сгорел окончательно
    agent->type = BURNED;
  }
  // Иначе:
  //  - есть топливо, но прямо сейчас вспышек нет → оставляем type как был
  //    (NORMAL, если ещё не загорался; или BURNING, если уже горел)
}

void SimulationStep(Agent *agents, int count, float dt,
                    const glm::vec3 &wind_force,
                    const FireSimulationParams &params) {
  cudaDeviceSynchronize();
  dt *= 0.1f;

  const int block_size = 256;
  const int grid_size = (count + block_size - 1) / block_size;

  float3 f = {wind_force.x, wind_force.y, wind_force.z};

  // Передаём params в ядро (пока оно его внутри не использует)
  update_agents_kernel<<<grid_size, block_size>>>(agents, count, dt, f, params);
  cudaDeviceSynchronize();

  cudaError_t err = cudaGetLastError();
  if (err != cudaSuccess) {
    printf("CUDA error: %s\n", cudaGetErrorString(err));
  }
}
