// ui/panels/FireDebugPanel.cc
#include "FireDebugPanel.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <imgui.h>

#include "core/Input.h"
#include "core/scene/Scene.h"
#include "math/ScreenRay.h"

namespace ui {

namespace {

// простой raycast по террейну без anchorY
bool RaycastTerrainSimple(const engine::Terrain& T,
                          const glm::vec3& ro,
                          const glm::vec3& rd,
                          glm::vec3& outHit) {
  auto f = [&](float t) {
    glm::vec3 p = ro + rd * t;
    return p.y - T.GetHeightAt(p.x, p.z);
  };

  const float T_MAX = 5000.0F;
  const float STEP  = 2.0F;

  float t0 = 0.0F;
  float v0 = f(t0);

  for (float t = STEP; t <= T_MAX; t += STEP) {
    float v1 = f(t);
    bool signChange = (v0 > 0.0F && v1 <= 0.0F) || (v0 < 0.0F && v1 >= 0.0F);
    if (signChange) {
      float a = t - STEP;
      float b = t;
      // бинпоиск
      for (int i = 0; i < 24; ++i) {
        float m  = 0.5F * (a + b);
        float vm = f(m);
        if ((v0 > 0.0F && vm > 0.0F) || (v0 < 0.0F && vm < 0.0F)) {
          a = m;
          v0 = vm;
        } else {
          b = m;
        }
      }
      float thit = 0.5F * (a + b);
      outHit = ro + rd * thit;
      return true;
    }
    t0 = t;
    v0 = v1;
  }
  return false;
}

} // namespace

void FireDebugPanel::Draw(EditorContext &ctx) {
  // 0) Всегда слушаем жест Ctrl+ЛКМ — он может открыть панель
  HandlePick(ctx);

  // 1) Если панель скрыта — даже не создаём ImGui-окно
  if (!m_visible) {
    return;
  }

  // 2) Окно с крестиком (ImGui меняет m_visible при нажатии X)
  if (!ImGui::Begin(Id(), &m_visible)) {
    ImGui::End();
    if (!m_visible) {
      m_hasSelection = false;
      m_selectedGpuIndex = -1;
    }
    return;
  }

  if (ctx.m_scene == nullptr) {
    ImGui::TextUnformatted("No scene.");
    ImGui::End();
    return;
  }

  // Пока окно открыто, можно пере-выбирать другого агента тем же жестом
  HandlePick(ctx);

  // Подтягиваем актуальное состояние с GPU для выбранного агента
  if (m_hasSelection) {
    auto agents = ctx.m_scene->LoadAgentsFromGPU();
    if (m_selectedGpuIndex >= 0 &&
        m_selectedGpuIndex < static_cast<int>(agents.size())) {
      m_agentSnapshot = agents[m_selectedGpuIndex];
    }
  }

  if (!m_hasSelection) {
    ImGui::TextUnformatted("Наведитесь курсором на объект в сцене\n"
                           "и нажмите Ctrl+ЛКМ для инспекции огня.");
  } else {
    DrawAgentInfo();
  }

  ImGui::End();

  // Если пользователь закрыл окно крестиком — очищаем выбор
  if (!m_visible) {
    m_hasSelection = false;
    m_selectedGpuIndex = -1;
  }
}

void FireDebugPanel::HandlePick(EditorContext &ctx) {
  using engine::InputSystem;

  if (InputSystem::Instance().UIMouseCaptured()) {
    return;
  }
  if ((ctx.m_window == nullptr) || (ctx.m_scene == nullptr)) {
    return;
  }

  const bool CTRL_DOWN =
      glfwGetKey(ctx.m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
      glfwGetKey(ctx.m_window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

  const bool LMB_PRESSED = InputSystem::Instance().GetMouseButton(
      engine::MouseButton::Left, engine::InputState::Pressed);

  if (!CTRL_DOWN || !LMB_PRESSED) {
    return;
}

  // Позиция курсора в окне
  double mx = 0.0;
  double my = 0.0;
  glfwGetCursorPos(ctx.m_window, &mx, &my);

  // окно -> координаты фреймбуфера (готовая хелпер-функция)
  const glm::vec2 FB = ctx.WindowToViewportFB(static_cast<float>(mx),
                                              static_cast<float>(my));

  // проверка, что внутри вьюпорта
  if (FB.x < 0.F || FB.y < 0.F ||
      FB.x >= static_cast<float>(ctx.vpW) ||
      FB.y >= static_cast<float>(ctx.vpH)) {
    return;
  }

  // луч из пикселя FB
  glm::vec3 ro{};
  glm::vec3 rd{};
  const auto FB_W = static_cast<float>(ctx.m_viewportW);
  const auto FB_H = static_cast<float>(ctx.m_viewportH);
  math::ScreenPointToRay(FB.x, FB.y, FB_W, FB_H, ctx.m_proj, ctx.m_view,
                         ro, rd);

  glm::vec3 hit{};
  bool got = false;

  const auto* T = ctx.m_scene->GetTerrain();
  if (T) {
    got = RaycastTerrainSimple(*T, ro, rd, hit);
  }
  if (!got) {
    got = math::IntersectRayPlaneY0(ro, rd, hit);
  }
  if (!got) {
    return;
  }

  // если есть террейн — подправим Y ещё раз (на случай фолбека по плоскости)
  if (T) {
    hit.y = T->GetHeightAt(hit.x, hit.z);
  }

  // поиск ближайшего агента по XZ
  float bestDist2 = 4.0f; // 2^2
  int   bestGpuIndex = -1;

  auto view = ctx.m_scene->GetSceneObjectsView();
  for (auto [entity, transform, render, link] : view.each()) {
    const float dx = transform.position.x - hit.x;
    const float dz = transform.position.z - hit.z;
    const float dist2 = dx * dx + dz * dz;
    if (dist2 < bestDist2) {
      bestDist2  = dist2;
      bestGpuIndex = link.gpu_index;
    }
  }

  if (bestGpuIndex < 0) {
    return;
}

  auto agents = ctx.m_scene->LoadAgentsFromGPU();
  if (bestGpuIndex >= 0 &&
      bestGpuIndex < static_cast<int>(agents.size())) {
    m_agentSnapshot    = agents[bestGpuIndex];
    m_selectedGpuIndex = bestGpuIndex;
    m_hasSelection     = true;
    m_pointIndex       = 0;
    m_visible          = true;
  }
}


void FireDebugPanel::DrawAgentInfo() {
  const Agent &a = m_agentSnapshot;
  const FireCollider &c = a.fire_collider;

  ImGui::SeparatorText("Selected Agent");

  ImGui::Text("GPU index: %d", m_selectedGpuIndex);
  ImGui::Text("Type: %d", static_cast<int>(a.type));
  ImGui::Text("Object id: %d", a.object_id);
  ImGui::Text("Overall intensity: %.3f", a.getOverallIntensity());
  ImGui::Text("Collider points: %d", c.point_count);
  ImGui::Separator();

  ImGui::Text("Material:");
  ImGui::Text("  Conductivity: %.3f", c.conductivity);
  ImGui::Text("  Ignition temp: %.1f", c.ignition_temp);

  // Соберём агрегатные значения по температурам и т.п.
  float minT = std::numeric_limits<float>::max();
  float maxT = -std::numeric_limits<float>::max();
  float sumT = 0.f;

  float minI = std::numeric_limits<float>::max();
  float maxI = -std::numeric_limits<float>::max();
  float sumI = 0.f;

  float minFuel = std::numeric_limits<float>::max();
  float maxFuel = -std::numeric_limits<float>::max();
  float sumFuel = 0.f;

  float minM = std::numeric_limits<float>::max();
  float maxM = -std::numeric_limits<float>::max();
  float sumM = 0.f;

  const int N = c.point_count;
  for (int i = 0; i < N; ++i) {
    const float T = c.temperatures[i];
    const float I = c.intensities[i];
    const float F = c.fuels[i];
    const float M = c.moisture[i];

    minT = std::min(minT, T);
    maxT = std::max(maxT, T);
    sumT += T;

    minI = std::min(minI, I);
    maxI = std::max(maxI, I);
    sumI += I;

    minFuel = std::min(minFuel, F);
    maxFuel = std::max(maxFuel, F);
    sumFuel += F;

    minM = std::min(minM, M);
    maxM = std::max(maxM, M);
    sumM += M;
  }

  const float avgT = (N > 0) ? sumT / N : 0.f;
  const float avgI = (N > 0) ? sumI / N : 0.f;
  const float avgFuel = (N > 0) ? sumFuel / N : 0.f;
  const float avgM = (N > 0) ? sumM / N : 0.f;

  ImGui::SeparatorText("Aggregate");
  ImGui::Text("Temperature:   min %.1f  avg %.1f  max %.1f  (threshold: %.1f)",
              minT, avgT, maxT, c.ignition_temp);
  ImGui::Text("Intensity:     min %.3f  avg %.3f  max %.3f", minI, avgI, maxI);
  ImGui::Text("Fuel:          min %.1f  avg %.1f  max %.1f", minFuel, avgFuel,
              maxFuel);
  ImGui::Text("Moisture:      min %.3f  avg %.3f  max %.3f", minM, avgM, maxM);

  if (maxT < c.ignition_temp) {
    ImGui::TextColored(ImVec4(1.f, 0.6f, 0.2f, 1.f),
                       "Not enough temperature for ignition.\n"
                       "Max T = %.1f, need >= %.1f (delta %.1f).",
                       maxT, c.ignition_temp, c.ignition_temp - maxT);
  } else {
    ImGui::TextColored(ImVec4(0.3f, 1.f, 0.3f, 1.f),
                       "Ignition threshold reached (max T = %.1f, thr = %.1f).",
                       maxT, c.ignition_temp);
  }

  ImGui::SeparatorText("Point details");

  if (N <= 0) {
    ImGui::TextUnformatted("No fire points.");
    return;
  }

  if (m_pointIndex >= N) {
    m_pointIndex = N - 1;
  }
  m_pointIndex = std::max(m_pointIndex, 0);

  ImGui::SliderInt("Point", &m_pointIndex, 0, N - 1);

  const int i = m_pointIndex;
  ImGui::Text("Point %d:", i);
  ImGui::Text("  Pos:      (%.2f, %.2f, %.2f)", c.heat_points[i].x,
              c.heat_points[i].y, c.heat_points[i].z);
  ImGui::Text("  Temp:     %.2f", c.temperatures[i]);
  ImGui::Text("  Intensity %.3f", c.intensities[i]);
  ImGui::Text("  Fuel:     %.1f", c.fuels[i]);
  ImGui::Text("  Moisture: %.3f", c.moisture[i]);
}




} // namespace ui
