// ui/tools/FoliageBrushTool.cc
#include "FoliageBrushTool.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <cmath>
#include <numbers>
#include "core/Input.h"
#include "core/scene/Scene.h"
#include "math/ScreenRay.h"
#include "ui/EditorContext.h"
#include "ui/ui_utils/SpatialGrid.h"

using engine::InputState;
using engine::MouseButton;
using Grid = ui::util::SpatialGrid;

namespace ui {

static float ComputeAnchorY(const ui::EditorContext& ctx) {
  float anchorY = 0.f;
  if ((ctx.m_scene != nullptr) && !ctx.State().m_selectedPrefab.empty()) {
    const auto& id = ctx.State().m_selectedPrefab;
    const auto& P = ctx.m_scene->GetPrefabSystem().GetPrefab(id);
    anchorY = -P.m_bounds_min.y * P.m_modelScale.y;
  }
  return anchorY;
}

static bool RaycastTerrain(const engine::Terrain& T, const glm::vec3& ro, const glm::vec3& rd,
                           float anchorY, glm::vec3& outHit) {
  auto f = [&](float t) {
    glm::vec3 p = ro + rd * t;
    return p.y - (T.GetHeightAt(p.x, p.z) + anchorY);
  };
  const float T_MAX = 5000.F;
  const float STEP = 2.F;
  float t0 = 0.F;
  float v0 = f(t0);
  for (float t = STEP; t <= T_MAX; t += STEP) {
    float v1 = f(t);
    if ((v0 > 0.F && v1 <= 0.F) || (v0 < 0.F && v1 >= 0.F)) {
      float a = t - STEP;
      float b = t;
      for (int i = 0; i < 24; ++i) {
        float m = 0.5F * (a + b);
        float vm = f(m);
        if ((v0 > 0.f && vm > 0.f) || (v0 < 0.f && vm < 0.f)) {
          a = m;
          v0 = vm;
        } else {
          b = m;
        }
      }
      float thit = 0.5f * (a + b);
      outHit = ro + rd * thit;
      return true;
    }
    t0 = t;
    v0 = v1;
  }
  return false;
}

void FoliageBrushTool::OnActivate(EditorContext& /*ctx*/) {
  m_preview.clear();
  m_dragging = false;
  EnsureSeed();
}

void FoliageBrushTool::EnsureSeed() {
  m_rng.seed(m_settings.seed);
}

bool FoliageBrushTool::ScreenToGround(EditorContext& ctx, float mx, float my, glm::vec3& outHit) {
  // как в PlaceOnGroundTool: окно → viewport → луч → пересечение Y=0
  double dx = 0.0;
  double dy = 0.0;
  dx = mx;
  dy = my;

  const glm::vec2 vp = ctx.WindowToViewportFB(static_cast<float>(dx), static_cast<float>(dy));
  if (vp.x < 0.f || vp.y < 0.f || vp.x >= (float)ctx.vpW || vp.y >= (float)ctx.vpH) {
    return false;
  }

  const float x_ndc = ((vp.x / static_cast<float>(ctx.vpW)) * 2.F) - 1.F;
  const float y_ndc = 1.f - (vp.y / (float)ctx.vpH) * 2.f;

  const glm::mat4& V = ctx.m_view;
  const glm::mat4& P = ctx.m_proj;
  const glm::mat4 invVP = glm::inverse(P * V);

  glm::vec4 p0 = invVP * glm::vec4(x_ndc, y_ndc, -1.f, 1.f);
  glm::vec4 p1 = invVP * glm::vec4(x_ndc, y_ndc, 1.f, 1.f);
  p0 /= p0.w;
  p1 /= p1.w;

  const glm::vec3 ro = glm::vec3(p0);
  const glm::vec3 rd = glm::normalize(glm::vec3(p1) - ro);

  if (std::abs(rd.y) < 1e-6f)
    return false;
  const float t = -ro.y / rd.y;
  if (t < 0.f)
    return false;

  outHit = ro + rd * t;
  return true;
}

void FoliageBrushTool::OnUpdate(EditorContext& ctx) {
  const bool uiCaptured = engine::InputSystem::Instance().UIMouseCaptured();

  // --- обрабатываем ввод только когда мышь не захвачена UI ---
  // одиночный клик — назначаем центр
  if (!uiCaptured) {
    if (engine::InputSystem::Instance().GetMouseButton(MouseButton::Left, InputState::Pressed)) {
      double mx;
      double my;
      glfwGetCursorPos(ctx.m_window, &mx, &my);

      // строим луч так же, как сейчас
      const glm::vec2 fb = ctx.WindowToViewportFB((float)mx, (float)my);
      glm::vec3 ro;
      glm::vec3 rd;
      math::ScreenPointToRay(fb.x, fb.y, (float)ctx.m_viewportW, (float)ctx.m_viewportH, ctx.m_proj,
                             ctx.m_view, ro, rd);

      glm::vec3 hit{};
      bool ok = false;
      const auto* T = ctx.m_scene ? ctx.m_scene->GetTerrain() : nullptr;
      const float anchorY = ComputeAnchorY(ctx);

      if (T)
        ok = RaycastTerrain(*T, ro, rd, anchorY, hit);
      if (!ok)
        ok = math::IntersectRayPlaneY0(ro, rd, hit);  // фолбек

      if (ok) {
        m_center = hit;
        m_center.y = T ? T->GetHeightAt(hit.x, hit.z) + anchorY : 0.f;
        m_hasCenter = true;
        // пересобираем предпросмотр как и раньше
        if (m_settings.shape == Shape::Circle) {
          m_anchor = m_center;
          m_current = m_center + glm::vec3(m_settings.radius, 0, 0);
          RebuildPreviewCircle();
        } else {
          m_anchor = m_center - glm::vec3(m_settings.width * 0.5f, 0, m_settings.height * 0.5f);
          m_current = m_center + glm::vec3(m_settings.width * 0.5f, 0, m_settings.height * 0.5f);
          RebuildPreviewRect();
        }
      }
    }

    // хоткеи
    if (engine::InputSystem::Instance().GetKey(engine::KeyCode::Enter, InputState::Pressed)) {
      ApplyPlacement(*ctx.m_scene);
    }
    if (engine::InputSystem::Instance().GetKey(engine::KeyCode::Escape, InputState::Pressed)) {
      m_preview.clear();
      m_hasCenter = false;
    }
  }

  // --- оверлей (контур + точки) рисуем всегда ---
  DrawOverlay(ctx);
}

void FoliageBrushTool::DrawControls(EditorContext& ctx) {
  bool dirty = false;
  int shape = (m_settings.shape == Shape::Circle ? 0 : 1);
  ImGui::TextUnformatted("Shape");
  dirty |= ImGui::RadioButton("Circle", &shape, 0);
  ImGui::SameLine();
  dirty |= ImGui::RadioButton("Rect", &shape, 1);
  m_settings.shape = (shape == 0 ? Shape::Circle : Shape::Rect);
  if (m_settings.shape == Shape::Circle) {
    dirty |= ImGui::SliderFloat("Radius (m)", &m_settings.radius, 1.f, 400.f);
  } else {
    dirty |= ImGui::SliderFloat("Width (m)", &m_settings.width, 1.f, 800.f);
    dirty |= ImGui::SliderFloat("Height (m)", &m_settings.height, 1.f, 800.f);
  }
  ImGui::Separator();
  dirty |= ImGui::SliderFloat("Density (per m^2)", &m_settings.density, 0.0f, 1.0f);
  dirty |= ImGui::SliderFloat("Min Spacing (m)", &m_settings.minSpacing, 0.0f, 20.0f);
  dirty |= ImGui::SliderFloat("Jitter [0..1]", &m_settings.jitter, 0.0f, 1.0f);
  ImGui::Separator();
  dirty |= ImGui::InputInt("Seed", (int*)&m_settings.seed);
  dirty |= ImGui::SliderFloat("Scale min", &m_settings.scaleMin, 0.2f, 10.0f);
  dirty |= ImGui::SliderFloat("Scale max", &m_settings.scaleMax, 0.2f, 10.0f);
  dirty |= ImGui::SliderFloat("Yaw min", &m_settings.yawMinDeg, 0.f, 360.f);
  dirty |= ImGui::SliderFloat("Yaw max", &m_settings.yawMaxDeg, 0.f, 360.f);

  // ----- ВЫБОР ПРЕФАБА ИЗ СПИСКА -----
  ImGui::Separator();
  ImGui::TextUnformatted("Prefab");

  if (ctx.m_scene) {
    auto& prefabSystem = ctx.m_scene->GetPrefabSystem();
    auto prefabNames = prefabSystem.GetPrefabNames();  // все префабы

    if (!prefabNames.empty()) {
      // если текущий prefab пустой или не найден — берём первый
      int currentIndex = 0;
      if (!m_settings.prefab.empty()) {
        auto it = std::find(prefabNames.begin(), prefabNames.end(), m_settings.prefab);
        if (it != prefabNames.end()) {
          currentIndex = static_cast<int>(std::distance(prefabNames.begin(), it));
        }
      } else {
        m_settings.prefab = prefabNames[0];
        dirty = true;
      }

      // Комбо по имени префаба
      const char* preview = prefabNames[currentIndex].c_str();
      if (ImGui::BeginCombo("##PrefabCombo", preview)) {
        for (int i = 0; i < static_cast<int>(prefabNames.size()); ++i) {
          const bool isSelected = (i == currentIndex);
          const std::string& name = prefabNames[i];

          // Можно тут же скрывать "огненные" префабы, если не нужны:
          // if (IsFireName(name)) continue;

          if (ImGui::Selectable(name.c_str(), isSelected)) {
            currentIndex = i;
            m_settings.prefab = name;
            dirty = true;
          }
          if (isSelected)
            ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
    } else {
      ImGui::TextUnformatted("No prefabs loaded");
    }
  } else {
    ImGui::TextUnformatted("Scene not available");
  }

  if (ImGui::Button("Re-seed")) {
    EnsureSeed();
    dirty = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Apply")) {
    if (ctx.m_scene) {
      ApplyPlacement(*ctx.m_scene);
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Clear")) {
    m_preview.clear();
    m_hasCenter = false;
  }
  ImGui::Separator();
  ImGui::Text("Preview points: %zu", m_preview.size());

  if (dirty && m_hasCenter) {
    if (m_settings.shape == Shape::Circle) {
      m_anchor = m_center;
      m_current = m_center + glm::vec3(m_settings.radius, 0, 0);
      RebuildPreviewCircle();
    } else {
      m_anchor = m_center - glm::vec3(m_settings.width * 0.5f, 0, m_settings.height * 0.5f);
      m_current = m_center + glm::vec3(m_settings.width * 0.5f, 0, m_settings.height * 0.5f);
      RebuildPreviewRect();
    }
  }
}

// ——— генераторы предпросмотра ———

void FoliageBrushTool::RebuildPreviewCircle() {
  m_preview.clear();
  const glm::vec2 c = {m_anchor.x, m_anchor.z};
  const glm::vec2 q = {m_current.x, m_current.z};
  const float R = (m_dragging ? glm::length(q - c) : m_settings.radius);

  const float area = std::numbers::pi_v<float> * R * R;
  const float expectedN = area * m_settings.density;

  const float cell = std::max(m_settings.minSpacing, 0.5f);
  std::uniform_real_distribution<float> uni(-0.5f, 0.5f);

  Grid grid;
  grid.SetCell(cell);
  m_preview.reserve(static_cast<size_t>(std::max(0.f, expectedN)));

  for (float x = -R; x <= R; x += cell) {
    for (float y = -R; y <= R; y += cell) {
      if (x * x + y * y > R * R)
        continue;
      glm::vec2 p = {c.x + x, c.y + y};
      p += glm::vec2(uni(m_rng), uni(m_rng)) * (cell * m_settings.jitter);
      if (m_settings.minSpacing <= 0.0f || grid.Accept(p, m_settings.minSpacing)) {
        m_preview.emplace_back(p.x, 0.f, p.y);
      }
    }
  }

  if (expectedN > 1 && m_preview.size() > static_cast<size_t>(expectedN)) {
    m_preview.resize(static_cast<size_t>(expectedN));
  }

  // (опционально)
  constexpr size_t kMaxPreviewPoints = 120'000;
  if (m_preview.size() > kMaxPreviewPoints)
    m_preview.resize(kMaxPreviewPoints);
}

void FoliageBrushTool::RebuildPreviewRect() {
  m_preview.clear();
  const glm::vec2 a = {m_anchor.x, m_anchor.z};
  const glm::vec2 b = {m_current.x, m_current.z};
  const glm::vec2 minp = {std::min(a.x, b.x), std::min(a.y, b.y)};
  const glm::vec2 maxp = {std::max(a.x, b.x), std::max(a.y, b.y)};
  const glm::vec2 size = maxp - minp;

  const float area = size.x * size.y;
  const float expectedN = area * m_settings.density;

  const float cell = std::max(m_settings.minSpacing, 0.5f);
  std::uniform_real_distribution<float> uni(-0.5f, 0.5f);

  Grid grid;
  grid.SetCell(cell);
  m_preview.reserve(static_cast<size_t>(std::max(0.f, expectedN)));

  for (float x = minp.x; x <= maxp.x; x += cell) {
    for (float y = minp.y; y <= maxp.y; y += cell) {
      glm::vec2 p = {x, y};
      p += glm::vec2(uni(m_rng), uni(m_rng)) * (cell * m_settings.jitter);
      if (m_settings.minSpacing <= 0.0f || grid.Accept(p, m_settings.minSpacing)) {
        m_preview.emplace_back(p.x, 0.f, p.y);
      }
    }
  }

  if (expectedN > 1 && m_preview.size() > static_cast<size_t>(expectedN)) {
    m_preview.resize(static_cast<size_t>(expectedN));
  }

  // (опционально)
  constexpr size_t kMaxPreviewPoints = 120'000;
  if (m_preview.size() > kMaxPreviewPoints)
    m_preview.resize(kMaxPreviewPoints);
}

static bool IsFireName(std::string_view id) {
  // временная эвристика — до появления нормального category/flags в префабе
  return id.find("fire") != std::string_view::npos || id.find("flame") != std::string_view::npos;
}
// ——— размещение ———

void FoliageBrushTool::ApplyPlacement(engine::Scene& scene) {
  if (m_preview.empty())
    return;
  if (IsFireName(m_settings.prefab)) {
    m_preview.clear();
    return;  // тихо выходим (можно логнуть/мигнуть тултипом)
  }

  std::uniform_real_distribution<float> unif01(0.f, 1.f);
  const float smin = m_settings.scaleMin, smax = m_settings.scaleMax;
  const float yawMin = glm::radians(m_settings.yawMinDeg);
  const float yawMax = glm::radians(m_settings.yawMaxDeg);

  for (auto& p : m_preview) {
    // scale (для «разной высоты» используешь масштаб по Y, но я даю униформ по всем осям)
    const float t = unif01(m_rng);
    const float s = smin + (smax - smin) * t;

    // поворот по Y (если модели поддерживают, можно хранить как часть SceneObject при расширении)
    (void)yawMin;
    (void)yawMax;  // пока не сохраняем углы — при надобности расширь SceneObject

    scene.AddTree(p.x, p.z, m_settings.prefab, s);
  }

  m_preview.clear();
}

// FoliageBrushTool.cc
bool FoliageBrushTool::ProjectToScreen(const EditorContext& ctx, const glm::vec3& p,
                                       glm::vec2& out_px) {
  const glm::vec4 clip = ctx.m_proj * ctx.m_view * glm::vec4(p, 1.0f);
  if (clip.w <= 0.0f)
    return false;
  const glm::vec3 ndc = glm::vec3(clip) / clip.w;  // [-1..1]
  if (ndc.x < -1.f || ndc.x > 1.f || ndc.y < -1.f || ndc.y > 1.f)
    return false;
  // в пиксели framebuffer
  const float x = (ndc.x * 0.5f + 0.5f) * (float)ctx.vpW;
  const float y = (1.f - (ndc.y * 0.5f + 0.5f)) * (float)ctx.vpH;
  out_px = {x, y};
  return true;
}

void FoliageBrushTool::DrawOverlay(EditorContext& ctx) {
  auto* dl = ImGui::GetBackgroundDrawList();  // рисуем под UI (можно Foreground)
  const ImU32 COL_OUTLINE = IM_COL32(80, 200, 120, 200);
  const ImU32 COL_POINTS = IM_COL32(80, 200, 120, 180);

  const auto* T = ctx.m_scene ? ctx.m_scene->GetTerrain() : nullptr;
  const float ANCHOR_Y = ComputeAnchorY(ctx);
  auto lift = [&](glm::vec3 p) {
    if (T)
      p.y = T->GetHeightAt(p.x, p.z) + ANCHOR_Y;
    return p;
  };
  // контур области
  std::vector<ImVec2> poly;
  poly.reserve(128);

  if (m_hasCenter) {
    if (m_settings.shape == Shape::Circle) {
      const float R =
          (m_dragging ? glm::length(glm::vec2(m_current.x - m_anchor.x, m_current.z - m_anchor.z))
                      : m_settings.radius);
      const int SEG = 64;
      for (int i = 0; i < SEG; i++) {
        float a = (float)i / SEG * 2.f * std::numbers::pi_v<float>;
        glm::vec3 pw = m_center + glm::vec3(std::cos(a) * R, 0.f, std::sin(a) * R);
        pw = lift(pw);
        glm::vec2 ps;
        if (ProjectToScreen(ctx, pw, ps))
          poly.emplace_back(ps.x, ps.y);
      }
    } else {  // Rect — тесселированная рамка по рельефу
      const float w = m_settings.width;
      const float h = m_settings.height;
      const glm::vec3 c = m_center;  // y не важен — lift() выставит

      // сколько сегментов на метр (подбери; 0.5…2 — ок)
      const float segPerMeter = 1.0f;
      const int segX = std::max(4, (int)std::ceil(w * segPerMeter));
      const int segZ = std::max(4, (int)std::ceil(h * segPerMeter));

      auto push_proj = [&](const glm::vec3& pw) {
        glm::vec2 s;
        if (ProjectToScreen(ctx, pw, s))
          poly.emplace_back(s.x, s.y);
      };

      poly.clear();
      poly.reserve(2 * (segX + segZ) * 2);

      // Левая кромка: (-w/2, -h/2) → (-w/2, +h/2)
      for (int i = 0; i <= segZ; ++i) {
        float t = (float)i / segZ;
        glm::vec3 p = c + glm::vec3(-w * 0.5f, 0.f, -h * 0.5f + t * h);
        push_proj(lift(p));
      }
      // Верхняя: (-w/2, +h/2) → (+w/2, +h/2)
      for (int i = 1; i <= segX; ++i) {
        float t = (float)i / segX;
        glm::vec3 p = c + glm::vec3(-w * 0.5f + t * w, 0.f, +h * 0.5f);
        push_proj(lift(p));
      }
      // Правая: (+w/2, +h/2) → (+w/2, -h/2)
      for (int i = 1; i <= segZ; ++i) {
        float t = (float)i / segZ;
        glm::vec3 p = c + glm::vec3(+w * 0.5f, 0.f, +h * 0.5f - t * h);
        push_proj(lift(p));
      }
      // Нижняя: (+w/2, -h/2) → (-w/2, -h/2)
      for (int i = 1; i <= segX; ++i) {
        float t = (float)i / segX;
        glm::vec3 p = c + glm::vec3(+w * 0.5f - t * w, 0.f, -h * 0.5f);
        push_proj(lift(p));
      }
    }
  }

if (poly.size() >= 2) {
  const bool isFullCircle = (m_settings.shape == Shape::Circle && poly.size() == 64);
  dl->AddPolyline(poly.data(),
                  (int)poly.size(),
                  COL_OUTLINE,
                  isFullCircle ? ImDrawFlags_Closed : 0,
                  2.0f);
}

  // точки предпросмотра
  // при очень больших количествах точек можно прореживать stride'ом
  const size_t stride = (m_preview.size() > 20000 ? 3 : 1);
  for (size_t i = 0; i < m_preview.size(); i += stride) {
    const auto& p = m_preview[i];
    glm::vec3 pw = lift(p);
    glm::vec2 px;
    if (ProjectToScreen(ctx, pw, px)) {
      dl->AddCircleFilled(ImVec2(px.x, px.y), 2.0f, COL_POINTS, 8);
    }
  }
}

}  // namespace ui
