#include "TimePanel.h"

#include <imgui.h>
#include <cstdio>      // std::snprintf
#include <ctime>       // std::tm

#include "core/scene/Scene.h"

namespace ui {

void TimePanel::Draw(EditorContext &ctx) {
  if (!m_visible) {
    return;
  }

  if (!ImGui::Begin(Id(), &m_visible)) {
    ImGui::End();
    return;
  }

  if (ctx.m_scene == nullptr) {
    ImGui::TextUnformatted("No scene.");
    ImGui::End();
    return;
  }

  auto &scene = *ctx.m_scene;
  auto &time  = scene.GetTime();

  // === блок: текущее время симуляции ===
  ImGui::SeparatorText("Simulation Time");

  double simSec  = time.GetSimTime();
  double realSec = time.GetRealTimeSinceStart();

  int simTotal = static_cast<int>(simSec);
  int simH = simTotal / 3600;
  int simM = (simTotal / 60) % 60;
  int simS = simTotal % 60;

  ImGui::Text("Sim time:  %02d:%02d:%02d (%.1f s)",
              simH, simM, simS, static_cast<float>(simSec));
  ImGui::Text("Real time: %.1f s since start", static_cast<float>(realSec));

  // локальные "настенные" часы
  std::tm now = time.GetLocalCurrentTime();
  ImGui::Text("Wall clock: %02d:%02d:%02d",
              now.tm_hour, now.tm_min, now.tm_sec);

  // === управление скоростью ===
  ImGui::SeparatorText("Control");

  double scale = time.GetTimeScale();
  double minScale = 0.0;
  double maxScale = 10.0;
  if (ImGui::SliderScalar("Time scale", ImGuiDataType_Double,
                          &scale, &minScale, &maxScale, "%.2fx")) {
    time.SetTimeScale(scale);
  }

  bool paused = time.IsPaused();
  if (ImGui::Checkbox("Pause simulation", &paused)) {
    time.SetPaused(paused);
  }

  // быстрые кнопки
  if (ImGui::Button("0.5x")) {
    time.SetTimeScale(0.5);
  }
  ImGui::SameLine();
  if (ImGui::Button("1x")) {
    time.SetTimeScale(1.0);
  }
  ImGui::SameLine();
  if (ImGui::Button("2x")) {
    time.SetTimeScale(2.0);
  }
  ImGui::SameLine();
  if (ImGui::Button("4x")) {
    time.SetTimeScale(4.0);
  }

  if (ImGui::Button("Reset sim time")) {
    time.ResetSimTime();
  }

  // можно показать "фактическую скорость" относительно реального времени
  double lastReal = time.GetLastRealDt();
  double lastSim  = time.GetLastSimDt();
  if (lastReal > 1e-6) {
    double factor = lastSim / lastReal;
    ImGui::Text("Effective sim speed: %.2fx", factor);
  }

  ImGui::End();
}

} // namespace ui
