#pragma once

#include <cuda_runtime.h>
#include "agent.h"
#include "fire/FireSimulationParams.h"
#include "simulation_math.cuh"

// Чистая "физика": теплопередача, ignition, burning, смена типа
namespace FirePhysics {

using FireMath::Clamp01;
using FireMath::WindData;

// --- ComputeHeatTransfer ---

__device__ float ComputeHeatTransfer(float Tsrc, float Tdst,
                                     float distance,
                                     float conductivity,
                                     float wind_factor,   // -1..1
                                     float moisture,      // 0..1
                                     float ignition_temp,
                                     const FireSimulationParams& p)
{
  if (distance < 0.1f) {
    distance = 0.1f;
  }

  float base = conductivity * (Tsrc - Tdst) / (distance * distance);

  float wind_mul = 1.0F;
  if (wind_factor > 0.0F) {
    wind_mul += wind_factor * p.m_wind_heat_bonus;
  } else if (wind_factor < 0.0F) {
    wind_mul += wind_factor * 0.3F;
  }

  float heat = base * wind_mul;

  const float m = Clamp01(moisture);
  heat *= (1.0F - m * p.m_humidity_heat_dampen);

  const float AIR = Clamp01(p.m_air_humidity_factor);
  heat *= (1.0F - AIR);

  if (ignition_temp > 0.0f && heat > 0.0f) {
    if (Tdst < ignition_temp) {
      float norm = Clamp01(Tdst / ignition_temp);

      float boost =
        1.0f + (1.0f - norm) * (p.m_initial_spread_boost - 1.0f);

      float penalty =
        p.m_cold_material_penalty +
        (1.0f - p.m_cold_material_penalty) * norm;

      heat *= (boost * penalty);
    }
  }

  return heat;
}

// --- ApplyIgnition ---

__device__ void ApplyIgnition(
    float& intensity,
    float  T,
    float  fuel,
    float  moisture,
    float  ignition_temp,
    const FireSimulationParams& p,
    float  dt)
{
  if (fuel <= 0.0F) {
    return;
  }

  const float M = Clamp01(moisture);

  const float EFF_IGN =
      ignition_temp * (1.0F + M * p.m_humidity_ignition_factor);

  if (T > EFF_IGN) {
    const float EXCESS = T - EFF_IGN;
    float k = EXCESS / EFF_IGN;
    k = Clamp01(k);

    const float TARGET_INTENSITY = 0.2F + (0.8F * k);

    const float ignite_rate = 3.0f;

    float delta = TARGET_INTENSITY - intensity;
    if (delta > 0.0f) {
      intensity += delta * Clamp01(ignite_rate * dt);
    }

    if (intensity < p.m_fire_intensity_min) {
      intensity = p.m_fire_intensity_min;
    }
  }
}

// --- UpdateBurningPoints ---

__device__ void UpdateBurningPoints(
    Agent& agent,
    const WindData& wind,
    float dt,
    const FireSimulationParams& params)
{
  const float fire_eps = params.m_fire_intensity_min;
  const float max_I    = params.m_max_intensity;

  const float base_max_T =
      (agent.fire_collider.max_temperature > 0.0f)
      ? agent.fire_collider.max_temperature
      : params.m_max_temperature;

  const float wind_boost =
      1.0f + 0.2f * Clamp01(wind.m_strength / 10.0f);

  const float local_max_T = base_max_T * wind_boost;

  const int point_count =
      FireMath::ClampedPointCount(agent.fire_collider);

  for (int i = 0; i < point_count; ++i) {
    float& I = agent.fire_collider.intensities[i];
    float& F = agent.fire_collider.fuels[i];
    float& T = agent.fire_collider.temperatures[i];

    if (I > fire_eps) {
      const float wind_burn = 1.0f + wind.m_strength * 0.5f;
      const float wind_I    = 1.0f + wind.m_strength * 0.3f;

      F -= I * dt * params.m_burn_fuel_rate * wind_burn;
      I += dt * params.m_intensity_growth_rate * wind_I;
      if (I > max_I) I = max_I;

      T += I * params.m_heat_production * dt;
      if (T > local_max_T) T = local_max_T;

      if (F <= 0.0f) {
        F = 0.0f;
        I = 0.0f;
      }
    } else {
      const float ambient = params.m_ambient_temperature;
      T -= params.m_cooling_rate * (T - ambient) * dt;
      if (T < ambient) T = ambient;
    }
  }
}

// --- CoolBurnedAgent ---

__device__ void CoolBurnedAgent(
    Agent& agent,
    float dt,
    const FireSimulationParams& params)
{
  const float ambient = params.m_ambient_temperature;

  const int point_count =
      FireMath::ClampedPointCount(agent.fire_collider);

  for (int i = 0; i < point_count; ++i) {
    float& T = agent.fire_collider.temperatures[i];

    T -= params.m_cooling_rate * (T - ambient) * dt;
    if (T < ambient) {
      T = ambient;
    }

    agent.fire_collider.intensities[i] = 0.0f;
    agent.fire_collider.fuels[i]       = 0.0f;
  }
}

// --- IgnitePoints ---

__device__ void IgnitePoints(
    Agent& agent,
    float dt,
    const FireSimulationParams& params)
{
  const float fire_eps = params.m_fire_intensity_min;

  const int point_count =
      FireMath::ClampedPointCount(agent.fire_collider);

  for (int i = 0; i < point_count; ++i) {
    float& I = agent.fire_collider.intensities[i];
    float& F = agent.fire_collider.fuels[i];
    float& T = agent.fire_collider.temperatures[i];
    float  M = agent.fire_collider.moisture[i];

    const float global_moist =
        Clamp01(params.m_global_moisture_scale);
    M = Clamp01(M * global_moist);

    if (I > fire_eps || F <= 0.0f) {
      continue;
    }

    ApplyIgnition(I, T, F, M,
                  agent.fire_collider.ignition_temp,
                  params, dt);
  }
}

// --- UpdateAgentType ---

__device__ void UpdateAgentType(
    Agent& agent,
    const FireSimulationParams& params)
{
  bool has_fire = false;
  bool has_fuel = false;

  const int point_count =
      FireMath::ClampedPointCount(agent.fire_collider);

  for (int i = 0; i < point_count; ++i) {
    if (agent.fire_collider.intensities[i] > params.m_fire_intensity_min)
      has_fire = true;
    if (agent.fire_collider.fuels[i] > params.m_fuel_epsilon)
      has_fuel = true;
  }

  if (has_fire) {
    agent.type = FireAgentState::Burning;
  } else if (!has_fire && !has_fuel) {
    agent.type = FireAgentState::Burned;
  }
}

} // namespace FirePhysics
