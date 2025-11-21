// ui/panels/FireStatsPanel.cc
#include "FireStatsPanel.h"

#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <string>   
#include "core/scene/Scene.h"  // Scene, LoadAgentsFromGPU()

namespace ui {

namespace {

template <size_t N>
void DrawMetricPlot(const char* label,
                    const char* yUnit,   // размерность по оси Y
                    const std::array<float, N>& buffer,
                    int head,
                    int count,
                    float extraPadding = 0.1f,
                    bool symmetricZero = false,
                    float fixedMin = 0.0f,
                    float fixedMax = 0.0f,
                    bool useFixedRange = false)
{
  if (count < 2) {
    ImGui::Text("%s (Y: %s, X: time, s)", label, yUnit);
    ImGui::TextUnformatted("  Not enough data");
    return;
  }

  const int offset = (head - count + static_cast<int>(N)) % static_cast<int>(N);

  float minV = FLT_MAX;
  float maxV = -FLT_MAX;

  for (int i = 0; i < count; ++i) {
    const int idx = (offset + i) % static_cast<int>(N);
    const float v = buffer[idx];
    if (v < minV) minV = v;
    if (v > maxV) maxV = v;
  }

  if (useFixedRange) {
    minV = fixedMin;
    maxV = fixedMax;
  } else {
    if (symmetricZero) {
      const float m = std::max(std::abs(minV), std::abs(maxV));
      minV = -m;
      maxV = +m;
    } else if (minV == maxV) {
      const float base = (std::fabs(minV) > 1e-3f) ? std::fabs(minV) : 1.0f;
      minV = minV - base * 0.1f;
      maxV = maxV + base * 0.1f;
    } else {
      const float range = maxV - minV;
      minV -= range * extraPadding;
      maxV += range * extraPadding;
    }
  }

  // Последнее значение для подсказки
  const int lastIdx = (head - 1 + static_cast<int>(N)) % static_cast<int>(N);
  const float lastV = buffer[lastIdx];

  char overlay[64];
  std::snprintf(overlay, sizeof(overlay), "%.3f %s", lastV, yUnit);

  ImGui::Text("%s (Y: %s, X: time, s)", label, yUnit);
  ImGui::PlotLines(("##" + std::string(label)).c_str(),
                   buffer.data(),
                   count,
                   offset,
                   overlay,    // показывает текущий Y в единицах
                   minV,
                   maxV,
                   ImVec2(0, 80.0f));
}

} // namespace

void FireStatsPanel::Reset() {
  m_head    = 0;
  m_count   = 0;
  m_simTime = 0.0f;

  m_lastArea     = 0.0f;
  m_lastRadius   = 0.0f;
  m_lastSpeed    = 0.0f;
  m_lastMoisture = 0.0f;

  m_time.fill(0.0f);
  m_area.fill(0.0f);
  m_radius.fill(0.0f);
  m_speed.fill(0.0f);
  m_moisture.fill(0.0f);
}

void FireStatsPanel::Sample(EditorContext& ctx) {
  if (!ctx.m_scene) {
    return;
  }

  ImGuiIO& io = ImGui::GetIO();
  const float dt = io.DeltaTime > 0.0f ? io.DeltaTime : 0.0f;
  if (dt <= 0.0f || m_paused) {
    return;
  }

  m_simTime += dt;

  auto agents = ctx.m_scene->LoadAgentsFromGPU();

  bool hasBurning = false;
  float minX = 0.0f, maxX = 0.0f;
  float minZ = 0.0f, maxZ = 0.0f;

  float sumMoisture   = 0.0f;
  int   moistSamples  = 0;

  for (const Agent& a : agents) {
    if (!a.isBurning()) {
      continue;
    }

    const glm::vec3 pos = a.position.toGlm();
    if (!hasBurning) {
      hasBurning = true;
      minX = maxX = pos.x;
      minZ = maxZ = pos.z;
    } else {
      minX = std::min(minX, pos.x);
      maxX = std::max(maxX, pos.x);
      minZ = std::min(minZ, pos.z);
      maxZ = std::max(maxZ, pos.z);
    }

    const FireCollider& c = a.fire_collider;
    for (int i = 0; i < c.point_count; ++i) {
      // учитываем только "горящие" точки, чтобы влажность была по фронту
      if (c.intensities[i] > 0.05f) {
        sumMoisture += c.moisture[i];
        ++moistSamples;
      }
    }
  }

  float area   = 0.0f;
  float radius = 0.0f;
  float avgMoist = 0.0f;

  if (hasBurning) {
    const float dx = maxX - minX;
    const float dz = maxZ - minZ;
    area   = std::max(dx, 0.0f) * std::max(dz, 0.0f);

    // "радиус" как половина диагонали bbox
    const float diag = std::sqrt(dx * dx + dz * dz);
    radius = 0.5f * diag;

    if (moistSamples > 0) {
      avgMoist = sumMoisture / static_cast<float>(moistSamples);
    }
  }

  float speed = 0.0f;
  if (m_count > 0) {
    const int prevIndex = (m_head - 1 + HISTORY) % HISTORY;
    const float prevRadius = m_radius[prevIndex];
    speed = (radius - prevRadius) / dt;
  }

  // Запоминаем текущие значения для текстового вывода
  m_lastArea     = area;
  m_lastRadius   = radius;
  m_lastSpeed    = speed;
  m_lastMoisture = avgMoist;

  // Записываем в кольцевой буфер
  m_time[m_head]     = m_simTime;
  m_area[m_head]     = area;
  m_radius[m_head]   = radius;
  m_speed[m_head]    = speed;
  m_moisture[m_head] = avgMoist;

  m_head = (m_head + 1) % HISTORY;
  if (m_count < HISTORY) {
    ++m_count;
  }
}

void FireStatsPanel::Draw(EditorContext& ctx) {
  if (!ImGui::Begin(Id())) {
    ImGui::End();
    return;
  }

  // Управление
  if (ImGui::Button(m_paused ? "Resume" : "Pause")) {
    m_paused = !m_paused;
  }
  ImGui::SameLine();
  if (ImGui::Button("Reset")) {
    Reset();
  }

  ImGui::SeparatorText("Current values");
  ImGui::Text("Time:         %.1f s", m_simTime);
  ImGui::Text("Area (XZ):    %.2f m^2", m_lastArea);
  ImGui::Text("Radius:       %.2f m",   m_lastRadius);
  ImGui::Text("Front speed:  %.2f m/s", m_lastSpeed);
  ImGui::Text("Moisture:     %.3f",     m_lastMoisture);

  // Делаем сэмпл перед рисованием графиков (один раз за кадр)
  Sample(ctx);

if (m_count > 1) {
  ImGui::SeparatorText("Graphs");

  const int values_count = m_count;

  // Площадь: м^2 (площадь bbox по XZ)
  DrawMetricPlot("Area", "m^2",
                 m_area, m_head, values_count);

  // Скорость фронта: м/с, симметрия относительно 0
  DrawMetricPlot("Front speed", "m/s",
                 m_speed, m_head, values_count,
                 0.1f, true /*symmetricZero*/);

  // Влажность: безразмерная доля 0..1
  DrawMetricPlot("Average moisture", "fraction (0..1)",
                 m_moisture, m_head, values_count,
                 0.0f, false,
                 0.0f, 1.0f, true /*useFixedRange*/);

  // Радиус: метры
  DrawMetricPlot("Radius", "m",
                 m_radius, m_head, values_count);
} else {
  ImGui::Separator();
  ImGui::TextUnformatted("Недостаточно данных для графиков. Запусти симуляцию.");
}

  ImGui::End();
}

} // namespace ui
