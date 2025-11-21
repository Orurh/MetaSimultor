#pragma once

#include <cuda_runtime.h>
#include "agent.h"

// Вспомогательные математика и ветер
namespace FireMath {

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
  if (n < 0)
    n = 0;
  if (n > 64)
    n = 64;
  return n;
}

struct WindData {
  float3 m_dir;      // нормализованное направление
  float m_strength;  // длина исходного вектора
};

__device__ WindData MakeWindData(const float3& wind_force) {
  WindData w{};
  w.m_strength = sqrtf((wind_force.x * wind_force.x) + (wind_force.y * wind_force.y) + (wind_force.z * wind_force.z));

  if (w.m_strength > 0.001F) {
    w.m_dir.x = wind_force.x / w.m_strength;
    w.m_dir.y = wind_force.y / w.m_strength;
    w.m_dir.z = wind_force.z / w.m_strength;
  } else {
    w.m_dir.x = w.m_dir.y = w.m_dir.z = 0.0F;
  }

  return w;
}

}  // namespace FireMath
