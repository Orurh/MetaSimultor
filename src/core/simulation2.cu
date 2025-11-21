// #include "agent.h"
// #include "simulation.cuh"

// #include <cuda_runtime.h>
// #include <cuda_runtime_api.h>
// #include <device_launch_parameters.h>

// #include <cmath>
// #include <iostream>

// namespace {

// struct GridIndex {
//   int x;
//   int z;
// };

// __device__ int GetCellIndexDevice(float x, float z, const FireGridParams& grid, GridIndex* out_gi) {
//   int cell_x = static_cast<int>((x - grid.world_min_x) / grid.cell_size);
//   int cell_z = static_cast<int>((z - grid.world_min_z) / grid.cell_size);

//   // за пределами мира – вываливаемся
//   if (cell_x < 0 || cell_x >= grid.grid_size_x || cell_z < 0 || cell_z >= grid.grid_size_z) {
//     if (out_gi) {
//       out_gi->x = -1;
//       out_gi->z = -1;
//     }
//     return -1;
//   }

//   if (out_gi) {
//     out_gi->x = cell_x;
//     out_gi->z = cell_z;
//   }

//   return (cell_z * grid.grid_size_x) + cell_x;  // row-major
// }

// __global__ void BuildGridKernel(const Agent* agents, int count, FireGridParams grid, int* cell_heads, int* cell_next) {
//   const int idx = threadIdx.x + blockIdx.x * blockDim.x;
//   if (idx < 0 || idx >= count) {
//     return;
//   }

//   // На всякий случай — защита от нулевых указателей
//   if (!agents || !cell_heads || !cell_next) {
//     return;
//   }

//   const Float3 pos = agents[idx].position;

//   GridIndex gi{};
//   int cell_idx = GetCellIndexDevice(pos.x, pos.z, grid, &gi);

//   // Агент вне мира → просто помечаем его как "нет ячейки"
//   if (cell_idx < 0) {
//     cell_next[idx] = -1;
//     return;
//   }

//   // Сверх-защита: проверяем, что индекс ячейки не выходит за пределы грида.
//   // (Если FireGridParams на устройстве битый — мы хотя бы не полезем за память.)
//   const int max_cells = grid.grid_size_x * grid.grid_size_z;
//   if (cell_idx < 0 || cell_idx >= max_cells) {
//     // Что-то не так с grid — лучше безопасно вывалиться
//     cell_next[idx] = -1;
//     return;
//   }

//   // Вставляем агента в односвязный список этой ячейки
//   int old_head = atomicExch(&cell_heads[cell_idx], idx);
//   cell_next[idx] = old_head;  // -1, если там никого не было
// }

// // -----------------------------------------------------------------------------
// // Small math helpers
// // -----------------------------------------------------------------------------

// __device__ float Clamp01(float x) {
//   return fminf(1.0F, fmaxf(0.0F, x));
// }

// __device__ float Distance3d(float x1, float y1, float z1, float x2, float y2, float z2) {
//   const float dx = x1 - x2;
//   const float dy = y1 - y2;
//   const float dz = z1 - z2;
//   return sqrtf((dx * dx) + (dy * dy) + (dz * dz));
// }

// __device__ int ClampedPointCount(const FireCollider& c) {
//   int n = c.point_count;
//   if (n < 0)
//     n = 0;
//   if (n > 64)
//     n = 64;
//   return n;
// }

// struct WindData {
//   float3 m_dir;      // нормализованное направление
//   float m_strength;  // длина исходного вектора
// };

// __device__ WindData MakeWindData(const float3& wind_force) {
//   WindData w{};
//   w.m_strength = sqrtf((wind_force.x * wind_force.x) + (wind_force.y * wind_force.y) + (wind_force.z * wind_force.z));

//   if (w.m_strength > 0.001F) {
//     w.m_dir.x = wind_force.x / w.m_strength;
//     w.m_dir.y = wind_force.y / w.m_strength;
//     w.m_dir.z = wind_force.z / w.m_strength;
//   } else {
//     w.m_dir.x = w.m_dir.y = w.m_dir.z = 0.0F;
//   }

//   return w;
// }

// // -----------------------------------------------------------------------------
// // Теплопередача и воспламенение
// // -----------------------------------------------------------------------------

// // Базовый расчёт теплопередачи между точками
// __device__ float ComputeHeatTransfer(float Tsrc, float Tdst, float distance, float conductivity,
//                                      float wind_factor,  // -1..1
//                                      float moisture,     // 0..1, локальная влажность источника
//                                      float ignition_temp, const FireSimulationParams& p) {
//   if (distance < 0.1f) {
//     distance = 0.1f;
//   }

//   // Простая модель: проводимость * градиент температуры / r^2
//   float base = conductivity * (Tsrc - Tdst) / (distance * distance);

//   // Ветер: усиливаем вдоль ветра, чуть гасим против
//   float wind_mul = 1.0F;
//   if (wind_factor > 0.0F) {
//     wind_mul += wind_factor * p.m_wind_heat_bonus;
//   } else if (wind_factor < 0.0F) {
//     wind_mul += wind_factor * 0.3F;  // чуть ослабляем против ветра
//   }

//   float heat = base * wind_mul;

//   // Локальная влажность гасит теплопередачу
//   const float m = Clamp01(moisture);  // 0..1
//   heat *= (1.0F - m * p.m_humidity_heat_dampen);

//   // Глобальная влажность воздуха тоже немного гасит
//   const float AIR = Clamp01(p.m_air_humidity_factor);  // 0..1
//   heat *= (1.0F - AIR);

//   // --- NEW: мягко усиливаем начальный нагрев для холодных материалов ---
//   // ignition_temp > 0 — нормальная ситуация; если вдруг нет, не трогаем.
//   if (ignition_temp > 0.0f && heat > 0.0f) {
//     // const float half_ign = 0.5f * ignition_temp;

//     if (Tdst < ignition_temp) {
//       // Нормируем на 0..1 в диапазоне [0 .. ignition_temp]
//       float norm = Clamp01(Tdst / ignition_temp);  // 0 при очень холодном, 1 около порога

//       // Чем холоднее материал, тем больше буст (до m_initial_spread_boost)
//       float boost = 1.0f + (1.0f - norm) * (p.m_initial_spread_boost - 1.0f);

//       // И немного штрафуем совсем холодные материалы (чтобы не улететь)
//       float penalty = p.m_cold_material_penalty + (1.0f - p.m_cold_material_penalty) * norm;

//       heat *= (boost * penalty);
//     }
//   }

//   return heat;
// }

// // Проверка воспламенения точки по температуре
// __device__ void ApplyIgnition(float& intensity, float T, float fuel, float moisture, float ignition_temp, const FireSimulationParams& p,
//                               float dt) {
//   if (fuel <= 0.0F) {
//     return;
//   }

//   const float M = Clamp01(moisture);  // 0..1

//   // Чем выше влажность, тем выше эффективный порог воспламенения
//   const float EFF_IGN = ignition_temp * (1.0F + M * p.m_humidity_ignition_factor);

//   if (T > EFF_IGN) {
//     const float EXCESS = T - EFF_IGN;
//     float k = EXCESS / EFF_IGN;  // относительный "запас" температуры
//     k = Clamp01(k);

//     const float TARGET_INTENSITY = 0.2F + (0.8F * k);

//     // скорость "подъёма" к целевой интенсивности
//     const float ignite_rate = 3.0f;  // можно потом вынести в FireSimulationParams

//     float delta = TARGET_INTENSITY - intensity;
//     if (delta > 0.0f) {
//       // ТЕПЕРЬ используем настоящий dt, а не захардкоженный 0.016
//       intensity += delta * Clamp01(ignite_rate * dt);
//     }

//     if (intensity < p.m_fire_intensity_min) {
//       intensity = p.m_fire_intensity_min;
//     }
//   }
// }

// // -----------------------------------------------------------------------------
// // Обновление одной "деревяшки"
// // -----------------------------------------------------------------------------

// // Шаг горения внутри агента (расход топлива, рост/затухание интенсивности, нагрев/остывание)
// __device__ void UpdateBurningPoints(Agent& agent, const WindData& wind, float dt, const FireSimulationParams& params) {
//   const float fire_eps = params.m_fire_intensity_min;
//   const float max_I = params.m_max_intensity;

//   // --- максимум температуры для этого агента ---
//   // если у коллайдера задан свой max_temperature > 0, берём его,
//   // иначе используем глобальный из параметров
//   const float base_max_T = (agent.fire_collider.max_temperature > 0.0f) ? agent.fire_collider.max_temperature : params.m_max_temperature;

//   // --- поправка от силы ветра ---
//   // wind.m_strength примерно в м/с, нормируем на "10" как условный максимум
//   const float wind_boost = 1.0f + 0.2f * Clamp01(wind.m_strength / 10.0f);

//   const float local_max_T = base_max_T * wind_boost;

//   const int point_count = ClampedPointCount(agent.fire_collider);
//   for (int i = 0; i < point_count; ++i) {
//     float& I = agent.fire_collider.intensities[i];
//     float& F = agent.fire_collider.fuels[i];
//     float& T = agent.fire_collider.temperatures[i];

//     if (I > fire_eps) {
//       const float wind_burn = 1.0f + wind.m_strength * 0.5f;
//       const float wind_I = 1.0f + wind.m_strength * 0.3f;

//       F -= I * dt * params.m_burn_fuel_rate * wind_burn;
//       I += dt * params.m_intensity_growth_rate * wind_I;
//       if (I > max_I)
//         I = max_I;

//       T += I * params.m_heat_production * dt;

//       // ★ ограничиваем температуру сверху с учётом материала и ветра
//       if (T > local_max_T) {
//         T = local_max_T;
//       }

//       if (F <= 0.0f) {
//         F = 0.0f;
//         I = 0.0f;
//       }
//     } else {
//       const float ambient = params.m_ambient_temperature;
//       T -= params.m_cooling_rate * (T - ambient) * dt;
//       if (T < ambient)
//         T = ambient;
//     }
//   }
// }

// // Распространение огня между агентами с учётом ветра (по позициям, а не индексам)
// __device__ void SpreadBetweenAgents_Simple(Agent* agents, int self_index, int agent_count, const FireGridParams& grid,
//                                            const int* cell_heads, const int* cell_next, const WindData& wind, float dt,
//                                            const FireSimulationParams& params) {
//   const float fire_eps = params.m_fire_intensity_min;

//   Agent& self = agents[self_index];

//   int sp = ClampedPointCount(self.fire_collider);
//   if (sp <= 0)
//     return;

//   float maxI = 0.f, maxT = 0.f, avgM = 0.f;
//   for (int i = 0; i < sp; ++i) {
//     maxI = fmaxf(maxI, self.fire_collider.intensities[i]);
//     maxT = fmaxf(maxT, self.fire_collider.temperatures[i]);
//     avgM += self.fire_collider.moisture[i];
//   }
//   avgM /= sp;

//   if (maxI <= params.m_spread_intensity_threshold)
//     return;

//   Float3 spc = self.position;
//   float3 C{spc.x, spc.y, spc.z};

//   GridIndex gi;
//   if (GetCellIndexDevice(spc.x, spc.z, grid, &gi) < 0)
//     return;

//   float effM = Clamp01(avgM * Clamp01(params.m_global_moisture_scale));

//   constexpr int MAX_NEI = 64;

//   for (int dz = -1; dz <= 1; ++dz)
//     for (int dx = -1; dx <= 1; ++dx) {
//       int nx = gi.x + dx, nz = gi.z + dz;
//       if (nx < 0 || nx >= grid.grid_size_x || nz < 0 || nz >= grid.grid_size_z)
//         continue;

//       int cell_idx = nz * grid.grid_size_x + nx;

//       int processed = 0;
//       for (int oi = cell_heads[cell_idx]; oi != -1 && processed < MAX_NEI; oi = cell_next[oi]) {
//         if (oi == self_index)
//           continue;
//         if (oi < 0 || oi >= agent_count)
//           break;

//         Agent* other = &agents[oi];
//         if (other->type == FireAgentState::Burned)
//           continue;

//         Float3 op = other->position;
//         float3 diff{op.x - C.x, op.y - C.y, op.z - C.z};

//         float d2 = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
//         if (d2 <= 1e-6f)
//           continue;

//         float d = sqrtf(d2);
//         diff.x /= d;
//         diff.y /= d;
//         diff.z /= d;

//         float wind_dot = diff.x * wind.m_dir.x + diff.y * wind.m_dir.y + diff.z * wind.m_dir.z;

//         float wind_sp = (wind_dot > 0.3f) ? (1.f + wind.m_strength * 1.5f * wind_dot) : (wind_dot < -0.3f ? 0.5f : 1.f);

//         float max_d = params.m_base_spread_distance * wind_sp;
//         if (d >= max_d)
//           continue;

//         float df = 1.f - d / max_d;
//         if (df < 0)
//           df = 0;

//         int opcnt = ClampedPointCount(other->fire_collider);
//         if (opcnt <= 0)
//           continue;

//         float avgT = 0.f;
//         for (int j = 0; j < opcnt; ++j)
//           avgT += other->fire_collider.temperatures[j];
//         avgT /= opcnt;

//         float cond = other->fire_collider.conductivity > 0.f ? other->fire_collider.conductivity : params.m_base_conductivity;

//         float heat = ComputeHeatTransfer(maxT, avgT, d, cond, wind_dot, effM, other->fire_collider.ignition_temp, params);

//         float wb = (wind_dot > 0.1f) ? (1.f + wind.m_strength * 2.f * wind_dot) : (wind_dot < -0.1f ? 0.5f : 1.f);

//         heat *= df * wb * Clamp01(maxI) * params.m_cross_agent_spread_strength * dt;

//         if (heat > 0) {
//           for (int j = 0; j < opcnt; ++j) {
//             if (other->fire_collider.intensities[j] > fire_eps)
//               continue;
//             other->fire_collider.temperatures[j] += heat;
//           }
//         }

//         ++processed;
//       }
//     }
// }

// __device__ void SpreadBetweenAgents_Full(Agent* agents, int self_index, int agent_count, const FireGridParams& grid, const int* cell_heads,
//                                          const int* cell_next, const WindData& wind, float dt, const FireSimulationParams& params) {
//   Agent& self = agents[self_index];

//   const Float3 self_pos = self.position;
//   const float3 self_center{self_pos.x, self_pos.y, self_pos.z};

//   const float spread_threshold = params.m_spread_intensity_threshold;
//   const float fire_eps = params.m_fire_intensity_min;

//   GridIndex self_gi;
//   int self_cell_idx = GetCellIndexDevice(self_pos.x, self_pos.z, grid, &self_gi);
//   if (self_cell_idx < 0)
//     return;

//   const int self_points = ClampedPointCount(self.fire_collider);

//   for (int i = 0; i < self_points; ++i) {
//     if (self.fire_collider.intensities[i] <= spread_threshold)
//       continue;

//     const glm::vec3 point_pos(self.fire_collider.heat_points[i].x, self.fire_collider.heat_points[i].y,
//                               self.fire_collider.heat_points[i].z);

//     for (int dz = -1; dz <= 1; ++dz)
//       for (int dx = -1; dx <= 1; ++dx) {
//         int nx = self_gi.x + dx;
//         int nz = self_gi.z + dz;

//         if (nx < 0 || nx >= grid.grid_size_x || nz < 0 || nz >= grid.grid_size_z)
//           continue;

//         const int cell_idx = nz * grid.grid_size_x + nx;

//         for (int other_idx = cell_heads[cell_idx]; other_idx != -1; other_idx = cell_next[other_idx]) {
//           if (other_idx < 0 || other_idx >= agent_count)
//             break;
//           if (other_idx == self_index)
//             continue;

//           Agent* other = &agents[other_idx];
//           if (other->type == FireAgentState::Burned)
//             continue;

//           const Float3 other_pos = other->position;
//           const float3 other_center{other_pos.x, other_pos.y, other_pos.z};

//           float3 dir{other_center.x - self_center.x, other_center.y - self_center.y, other_center.z - self_center.z};

//           float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
//           if (len <= 0.001f)
//             continue;

//           dir.x /= len;
//           dir.y /= len;
//           dir.z /= len;

//           float wind_dot = dir.x * wind.m_dir.x + dir.y * wind.m_dir.y + dir.z * wind.m_dir.z;

//           float wind_dist = (wind_dot > 0.3f) ? (1.f + wind.m_strength * 2.f * wind_dot) : (wind_dot < -0.3f ? 0.7f : 1.f);

//           float max_check_dist = params.m_base_check_distance * wind_dist;
//           if (len >= max_check_dist)
//             continue;

//           float dist_factor = 1.f - (len / max_check_dist);
//           if (dist_factor < 0)
//             dist_factor = 0;

//           int other_points = ClampedPointCount(other->fire_collider);

//           for (int j = 0; j < other_points; ++j) {
//             if (other->fire_collider.intensities[j] > fire_eps)
//               continue;

//             glm::vec3 op(other->fire_collider.heat_points[j].x, other->fire_collider.heat_points[j].y,
//                          other->fire_collider.heat_points[j].z);

//             float pd = Distance3d(point_pos.x, point_pos.y, point_pos.z, op.x, op.y, op.z);

//             float wind_spread = (wind_dot > 0.3f) ? (1.f + wind.m_strength * 1.5f * wind_dot) : (wind_dot < -0.3f ? 0.5f : 1.f);

//             float max_spread = params.m_base_spread_distance * wind_spread;
//             if (pd >= max_spread)
//               continue;

//             float pd_factor = 1.f - pd / max_spread;
//             if (pd_factor < 0)
//               pd_factor = 0;

//             float wind_bias = (wind_dot > 0.1f) ? (1.f + wind.m_strength * 2.f * wind_dot) : (wind_dot < -0.1f ? 0.5f : 1.f);

//             const float Tsrc = self.fire_collider.temperatures[i];
//             float& Tdst = other->fire_collider.temperatures[j];

//             if (Tsrc <= Tdst)
//               continue;

//             float cond = (other->fire_collider.conductivity > 0) ? other->fire_collider.conductivity : params.m_base_conductivity;

//             float effM = Clamp01(self.fire_collider.moisture[i] * Clamp01(params.m_global_moisture_scale));

//             float heat = ComputeHeatTransfer(Tsrc, Tdst, pd, cond, wind_dot, effM, other->fire_collider.ignition_temp, params);

//             heat *= pd_factor * wind_bias * params.m_cross_agent_spread_strength * dt;

//             if (heat > 0)
//               Tdst += heat;
//           }
//         }
//       }
//   }
// }

// // Более "умная" быстрая версия:
// // - считаем minT по целевому агенту (самые холодные точки)
// // - считаем общий heat_total для пары агентов
// // - распределяем heat_total по точкам цели так, чтобы более холодные
// //   получали больше тепла (по весам)
// __device__ void SpreadBetweenAgents_ColdSpots(Agent* agents, int self_index, int agent_count, const FireGridParams& grid,
//                                               const int* cell_heads, const int* cell_next, const WindData& wind, float dt,
//                                               const FireSimulationParams& params) {
//   if (!cell_heads || !cell_next)
//     return;

//   const float fire_eps = params.m_fire_intensity_min;
//   Agent& self = agents[self_index];
//   const int self_points = ClampedPointCount(self.fire_collider);
//   if (self_points <= 0)
//     return;

//   // Агрегируем источник
//   float self_max_I = 0.0f;
//   float self_max_T = 0.0f;
//   float self_avg_moist = 0.0f;

//   for (int i = 0; i < self_points; ++i) {
//     self_max_I = fmaxf(self_max_I, self.fire_collider.intensities[i]);
//     self_max_T = fmaxf(self_max_T, self.fire_collider.temperatures[i]);
//     self_avg_moist += self.fire_collider.moisture[i];
//   }
//   self_avg_moist /= self_points;

//   if (self_max_I <= params.m_spread_intensity_threshold)
//     return;

//   const Float3 self_pos = self.position;
//   GridIndex gi;
//   if (GetCellIndexDevice(self_pos.x, self_pos.z, grid, &gi) < 0)
//     return;

//   const float global_moist = Clamp01(params.m_global_moisture_scale);
//   const float eff_moisture = Clamp01(self_avg_moist * global_moist);

//   for (int dz = -1; dz <= 1; ++dz) {
//     for (int dx = -1; dx <= 1; ++dx) {
//       int nx = gi.x + dx, nz = gi.z + dz;
//       if (nx < 0 || nx >= grid.grid_size_x || nz < 0 || nz >= grid.grid_size_z)
//         continue;

//       const int cell_idx = nz * grid.grid_size_x + nx;

//       for (int other_idx = cell_heads[cell_idx]; other_idx != -1; other_idx = cell_next[other_idx]) {
//         if (other_idx < 0 || other_idx >= agent_count || other_idx == self_index)
//           continue;

//         Agent* other = &agents[other_idx];
//         if (other->type == FireAgentState::Burned)
//           continue;

//         const Float3 other_pos = other->position;
//         float3 diff{other_pos.x - self_pos.x, other_pos.y - self_pos.y, other_pos.z - self_pos.z};

//         float dist_sq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
//         if (dist_sq <= 1e-6f)
//           continue;

//         float dist = sqrtf(dist_sq);
//         diff.x /= dist;
//         diff.y /= dist;
//         diff.z /= dist;

//         const float wind_dot = diff.x * wind.m_dir.x + diff.y * wind.m_dir.y + diff.z * wind.m_dir.z;

//         float wind_spread_effect = (wind_dot > 0.3f) ? (1.0f + wind.m_strength * 1.5f * wind_dot) : (wind_dot < -0.3f ? 0.5f : 1.0f);

//         float max_spread_distance = params.m_base_spread_distance * wind_spread_effect;
//         if (dist >= max_spread_distance)
//           continue;

//         float distance_factor = 1.0f - dist / max_spread_distance;
//         if (distance_factor < 0.0f)
//           distance_factor = 0.0f;

//         const int other_points = ClampedPointCount(other->fire_collider);
//         if (other_points <= 0)
//           continue;

//         // 🔥 ИСПРАВЛЕНИЕ: ищем minT по ВСЕМ точкам, даже горящим!
//         float minT = 1e9f;
//         bool has_valid_points = false;
//         for (int j = 0; j < other_points; ++j) {
//           float T = other->fire_collider.temperatures[j];
//           if (T < minT) {
//             minT = T;
//             has_valid_points = true;
//           }
//         }

//         if (!has_valid_points)
//           continue;  // только если вообще нет точек

//         const float conductivity =
//             (other->fire_collider.conductivity > 0.0f) ? other->fire_collider.conductivity : params.m_base_conductivity;

//         // 🔥 ИСПРАВЛЕНИЕ 2: используем minT для базового расчета
//         float base_heat =
//             ComputeHeatTransfer(self_max_T, minT, dist, conductivity, wind_dot, eff_moisture, other->fire_collider.ignition_temp, params);

//         float wind_bias = (wind_dot > 0.1f) ? (1.0f + wind.m_strength * 3.0f * wind_dot) : (wind_dot < -0.1f ? 0.3f : 1.0f);

//         const float intensity_factor = Clamp01(self_max_I);

//         // 🔥 ИСПРАВЛЕНИЕ 3: правильное распределение тепла
//         float heat_total = base_heat * distance_factor * wind_bias * intensity_factor * params.m_cross_agent_spread_strength * dt;

//         if (heat_total <= 0.0f)
//           continue;

//         // Распределяем heat_total по всем точкам с приоритетом холодных
//         const float Tref = minT + 50.0f;
//         float weight_sum = 0.0f;
//         float weights[64];

//         for (int j = 0; j < other_points; ++j) {
//           float Tj = other->fire_collider.temperatures[j];
//           float w = Tref - Tj;
//           if (w < 0.0f)
//             w = 0.0f;
//           weights[j] = w;
//           weight_sum += w;
//         }

//         if (weight_sum <= 1e-6f) {
//           // Равномерное распределение
//           float dT = heat_total / other_points;
//           for (int j = 0; j < other_points; ++j) {
//             other->fire_collider.temperatures[j] += dT;
//           }
//         } else {
//           // Распределение по весам
//           for (int j = 0; j < other_points; ++j) {
//             float share = weights[j] / weight_sum;
//             float dT = heat_total * share;
//             other->fire_collider.temperatures[j] += dT;
//           }
//         }
//       }
//     }
//   }
// }

// __device__ void SpreadBetweenAgents(Agent* agents, int self_index, int agent_count, const FireGridParams& grid, const int* cell_heads,
//                                     const int* cell_next, const WindData& wind, float dt, const FireSimulationParams& params) {
//   // Полная физика (медленно)
//   SpreadBetweenAgents_Full(agents, self_index, agent_count, grid, cell_heads, cell_next, wind, dt, params);

//   // ⚡ Быстрая версия (быстро)
//   // SpreadBetweenAgents_Simple(agents, self_index, agent_count, grid, cell_heads, cell_next, wind, dt, params);

//   // 🥷 Быстрая версия с учётом холодных точек (средне)
//   SpreadBetweenAgents_ColdSpots(agents, self_index, agent_count, grid, cell_heads, cell_next, wind, dt, params);
// }

// // Теплопередача внутри одного агента (крона/ствол)
// __device__ void SpreadWithinAgent(Agent& agent, float dt, const FireSimulationParams& params) {
//   const int point_count = ClampedPointCount(agent.fire_collider);
//   for (int i = 0; i < point_count; ++i) {
//     const glm::vec3 point_pos(agent.fire_collider.heat_points[i].x, agent.fire_collider.heat_points[i].y,
//                               agent.fire_collider.heat_points[i].z);

//     for (int j = 0; j < point_count; ++j) {
//       if (i == j) {
//         continue;
//       }

//       const glm::vec3 other_point_pos(agent.fire_collider.heat_points[j].x, agent.fire_collider.heat_points[j].y,
//                                       agent.fire_collider.heat_points[j].z);

//       const float point_dist = Distance3d(point_pos.x, point_pos.y, point_pos.z, other_point_pos.x, other_point_pos.y, other_point_pos.z);

//       const float max_local_dist = params.m_local_spread_distance;
//       if (point_dist >= max_local_dist) {
//         continue;
//       }

//       float distance_factor = 1.0f - point_dist / max_local_dist;
//       if (distance_factor < 0.0f) {
//         distance_factor = 0.0f;
//       }

//       const float Tsrc = agent.fire_collider.temperatures[i];
//       float& Tdst = agent.fire_collider.temperatures[j];

//       if (Tsrc <= Tdst) {
//         continue;
//       }

//       const float conductivity = agent.fire_collider.conductivity > 0.0f ? agent.fire_collider.conductivity : params.m_base_conductivity;

//       const float src_moisture = agent.fire_collider.moisture[i];
//       const float global_moist = Clamp01(params.m_global_moisture_scale);
//       const float eff_moisture = Clamp01(src_moisture * global_moist);

//       float heat = ComputeHeatTransfer(Tsrc, Tdst, point_dist, conductivity,
//                                        /*wind_factor=*/0.0f,  // внутри агента ветер не учитываем
//                                        eff_moisture,
//                                        agent.fire_collider.ignition_temp,  // порог этого же материала
//                                        params);

//       heat *= distance_factor * params.m_local_spread_strength * dt;

//       if (heat > 0.0f) {
//         Tdst += heat;
//       }
//     }
//   }
// }

// // Обновление типа агента на основе состояния точек
// __device__ void UpdateAgentType(Agent& agent, const FireSimulationParams& params) {
//   bool has_fire = false;
//   bool has_fuel = false;

//   const int point_count = ClampedPointCount(agent.fire_collider);
//   for (int i = 0; i < point_count; ++i) {
//     if (agent.fire_collider.intensities[i] > params.m_fire_intensity_min) {
//       has_fire = true;
//     }
//     if (agent.fire_collider.fuels[i] > params.m_fuel_epsilon) {
//       has_fuel = true;
//     }
//   }

//   if (has_fire) {
//     // Как только где-то загорелось — помечаем агент как горящий
//     agent.type = FireAgentState::Burning;
//   } else if (!has_fire && !has_fuel) {
//     // Топлива нет и нигде не горит — всё, агент сгорел окончательно
//     agent.type = FireAgentState::Burned;
//   }
//   // Иначе:
//   //  - есть топливо, но прямо сейчас вспышек нет → оставляем type как был
//   //    (NORMAL, если ещё не загорался; или BURNING, если уже горел)
// }

// }  // namespace

// // охлаждаем головешки
// __device__ void CoolBurnedAgent(Agent& agent, float dt, const FireSimulationParams& params) {
//   const float ambient = params.m_ambient_temperature;

//   const int point_count = ClampedPointCount(agent.fire_collider);
//   for (int i = 0; i < point_count; ++i) {
//     float& T = agent.fire_collider.temperatures[i];

//     // экспоненциальное охлаждение к окружающей
//     T -= params.m_cooling_rate * (T - ambient) * dt;
//     if (T < ambient) {
//       T = ambient;
//     }

//     // на всякий случай гасим всё
//     agent.fire_collider.intensities[i] = 0.0f;
//     agent.fire_collider.fuels[i] = 0.0f;
//   }
// }

// __device__ void IgnitePoints(Agent& agent, float dt, const FireSimulationParams& params) {
//   const float fire_eps = params.m_fire_intensity_min;

//   const int point_count = ClampedPointCount(agent.fire_collider);
//   for (int i = 0; i < point_count; ++i) {
//     float& I = agent.fire_collider.intensities[i];
//     float& F = agent.fire_collider.fuels[i];
//     float& T = agent.fire_collider.temperatures[i];
//     float M = agent.fire_collider.moisture[i];
//     const float global_moist = Clamp01(params.m_global_moisture_scale);
//     M = Clamp01(M * global_moist);

//     // уже горит или топлива нет — ignition не нужен
//     if (I > fire_eps || F <= 0.0f) {
//       continue;
//     }

//     ApplyIgnition(I, T, F, M, agent.fire_collider.ignition_temp, params, dt);
//   }
// }

// // -----------------------------------------------------------------------------
// // CUDA kernel
// // -----------------------------------------------------------------------------

// __global__ void UpdateAgentsKernel(Agent* agents, int count, float dt, float3 wind_force, FireSimulationParams params, FireGridParams grid,
//                                    const int* cell_heads, const int* cell_next) {
//   const int IDX = threadIdx.x + (blockIdx.x * blockDim.x);
//   if (IDX >= count) {
//     return;
//   }

//   Agent& agent = agents[IDX];
//   if (agent.type == FireAgentState::Burned) {
//     CoolBurnedAgent(agent, dt, params);
//     return;
//   }

//   const WindData WIND = MakeWindData(wind_force);

//   // 1. Обновляем горение с учетом ветра и температуры
//   UpdateBurningPoints(agent, WIND, dt, params);

//   // 2. Распространение огня между агентами (через грид)
//   SpreadBetweenAgents(agents, IDX, count, grid, cell_heads, cell_next, WIND, dt, params);

//   // 3. Теплопередача внутри одного агента (крона/ствол)
//   SpreadWithinAgent(agent, dt, params);

//   // 3.5. ОТДЕЛЬНЫЙ шаг воспламенения точек на основе финальной температуры
//   IgnitePoints(agent, dt, params);

//   // 4. Обновляем тип агента
//   UpdateAgentType(agent, params);
// }

// // -----------------------------------------------------------------------------
// // Host API
// // -----------------------------------------------------------------------------

// void SimulationStep(Agent* agents, int count, float dt, const glm::vec3& wind_force, const FireSimulationParams& params,
//                     const FireGridParams& grid) {
//   if (!agents || count <= 0) {
//     return;
//   }

//   constexpr int K_BLOCK_SIZE = 256;
//   const int GRID_SIZE = (count + K_BLOCK_SIZE - 1) / K_BLOCK_SIZE;

//   const float3 f{wind_force.x, wind_force.y, wind_force.z};

//   static int* d_cell_heads = nullptr;
//   static int* d_cell_next = nullptr;
//   static int allocated_agents = 0;
//   static int allocated_cells = 0;

//   const int num_cells = grid.grid_size_x * grid.grid_size_z;

//   // --- защита от странного грида ---
//   if (grid.grid_size_x <= 0 || grid.grid_size_z <= 0 || num_cells <= 0) {
//     // просто обновляем ядром без грида (если нужно — можно сделать fallback)
//     // или вообще return; чтобы не падать
//     return;
//   }

//   auto checkedMalloc = [](void** ptr, size_t bytes, const char* what) -> bool {
//     cudaError_t err = cudaMalloc(ptr, bytes);
//     if (err != cudaSuccess) {
//       std::cerr << "cudaMalloc failed for " << what << " (" << bytes << " bytes): " << cudaGetErrorString(err) << "\n";
//       *ptr = nullptr;
//       return false;
//     }
//     return true;
//   };

//   // d_cell_next
//   if (allocated_agents < count) {
//     if (d_cell_next)
//       cudaFree(d_cell_next);
//     if (!checkedMalloc((void**)&d_cell_next, count * sizeof(int), "d_cell_next")) {
//       allocated_agents = 0;
//       return;  // дальше не идём
//     }
//     allocated_agents = count;
//   }

//   // d_cell_heads
//   if (allocated_cells < num_cells) {
//     if (d_cell_heads)
//       cudaFree(d_cell_heads);
//     if (!checkedMalloc((void**)&d_cell_heads, num_cells * sizeof(int), "d_cell_heads")) {
//       allocated_cells = 0;
//       return;
//     }
//     allocated_cells = num_cells;
//   }

//   // Обнуляем (и next, и heads), чтобы не тащить мусор между кадрами
//   cudaMemset(d_cell_heads, 0xFF, num_cells * sizeof(int));  // -1
//   cudaMemset(d_cell_next, 0xFF, allocated_agents * sizeof(int));

//   cudaError_t err;

//   // 1) строим грид
//   BuildGridKernel<<<GRID_SIZE, K_BLOCK_SIZE>>>(agents, count, grid, d_cell_heads, d_cell_next);
//   err = cudaDeviceSynchronize();
//   if (err != cudaSuccess) {
//     printf("BuildGridKernel launch error: %s\n", cudaGetErrorString(err));
//     return;  // дальше идти смысла нет
//   }

//   // 2) основное ядро
//   UpdateAgentsKernel<<<GRID_SIZE, K_BLOCK_SIZE>>>(agents, count, dt, f, params, grid, d_cell_heads, d_cell_next);
//   err = cudaDeviceSynchronize();
//   if (err != cudaSuccess) {
//     printf("UpdateAgentsKernel launch error: %s\n", cudaGetErrorString(err));
//   }
// }
