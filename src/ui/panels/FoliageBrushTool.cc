// ui/tools/FoliageBrushTool.cc
#include "FoliageBrushTool.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <cmath>
#include "ui/EditorContext.h"
#include "core/Input.h"
#include "core/scene/Scene.h"

using engine::MouseButton;
using engine::InputState;

namespace ui {

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
  double dx = 0.0, dy = 0.0;
  dx = mx; dy = my;

  const glm::vec2 vp = ctx.WindowToViewportFB((float)dx, (float)dy);
  if (vp.x < 0.f || vp.y < 0.f || vp.x >= (float)ctx.vpW || vp.y >= (float)ctx.vpH) return false;

  const float x_ndc = (vp.x / (float)ctx.vpW) * 2.f - 1.f;
  const float y_ndc = 1.f - (vp.y / (float)ctx.vpH) * 2.f;

  const glm::mat4& V = ctx.m_view;
  const glm::mat4& P = ctx.m_proj;
  const glm::mat4  invVP = glm::inverse(P * V);

  glm::vec4 p0 = invVP * glm::vec4(x_ndc, y_ndc, -1.f, 1.f);
  glm::vec4 p1 = invVP * glm::vec4(x_ndc, y_ndc,  1.f, 1.f);
  p0 /= p0.w; p1 /= p1.w;

  const glm::vec3 ro = glm::vec3(p0);
  const glm::vec3 rd = glm::normalize(glm::vec3(p1) - ro);

  if (std::abs(rd.y) < 1e-6f) return false;
  const float t = -ro.y / rd.y;
  if (t < 0.f) return false;

  outHit = ro + rd * t;
  return true;
}

void FoliageBrushTool::OnUpdate(EditorContext& ctx) {
  if (engine::InputSystem::Instance().UIMouseCaptured()) return;

  // одиночный клик — назначаем центр
  if (engine::InputSystem::Instance().GetMouseButton(MouseButton::Left, InputState::Pressed)) {
    double mx, my; glfwGetCursorPos(ctx.m_window, &mx, &my);
    glm::vec3 hit;
    if (ScreenToGround(ctx, (float)mx, (float)my, hit)) {
      m_center = hit; m_center.y = 0.f;
      m_hasCenter = true;
      // перестраиваем предпросмотр от центра
      if (m_settings.shape == Shape::Circle) {
        // рассчитываем от центра и текущего радиуса
        m_anchor = m_center; m_current = m_center + glm::vec3(m_settings.radius,0,0);
        RebuildPreviewCircle();
      } else {
        m_anchor = m_center - glm::vec3(m_settings.width*0.5f,0,m_settings.height*0.5f);
        m_current= m_center + glm::vec3(m_settings.width*0.5f,0,m_settings.height*0.5f);
        RebuildPreviewRect();
      }
    }
  }

  // горячие клавиши
  if (engine::InputSystem::Instance().GetKey(engine::KeyCode::Enter, InputState::Pressed)) {
    ApplyPlacement(*ctx.m_scene);
  }
  if (engine::InputSystem::Instance().GetKey(engine::KeyCode::Escape, InputState::Pressed)) {
    m_preview.clear(); m_hasCenter = false;
  }

  // рисуем оверлей (контур + точки)
  DrawOverlay(ctx);
}

void FoliageBrushTool::DrawUI(EditorContext& ctx) {
  if (ImGui::Begin("Foliage Brush")) {
    int shape = (m_settings.shape == Shape::Circle ? 0 : 1);
    ImGui::TextUnformatted("Shape");
    ImGui::RadioButton("Circle", &shape, 0); ImGui::SameLine();
    ImGui::RadioButton("Rect",   &shape, 1);
    m_settings.shape = (shape == 0 ? Shape::Circle : Shape::Rect);

    if (m_settings.shape == Shape::Circle) {
      ImGui::SliderFloat("Radius (m)", &m_settings.radius, 1.f, 200.f);
    } else {
      ImGui::SliderFloat("Width (m)",  &m_settings.width,  1.f, 400.f);
      ImGui::SliderFloat("Height (m)", &m_settings.height, 1.f, 400.f);
    }

    ImGui::Separator();
    ImGui::SliderFloat("Density (per m^2)", &m_settings.density, 0.0f, 1.0f);
    ImGui::SliderFloat("Min Spacing (m)",   &m_settings.minSpacing, 0.0f, 10.0f);
    ImGui::SliderFloat("Jitter [0..1]",     &m_settings.jitter, 0.0f, 1.0f);

    ImGui::Separator();
    ImGui::InputInt  ("Seed", (int*)&m_settings.seed);
    ImGui::SliderFloat("Scale min", &m_settings.scaleMin, 0.2f, 5.0f);
    ImGui::SliderFloat("Scale max", &m_settings.scaleMax, 0.2f, 5.0f);
    ImGui::SliderFloat("Yaw min",   &m_settings.yawMinDeg, 0.f, 360.f);
    ImGui::SliderFloat("Yaw max",   &m_settings.yawMaxDeg, 0.f, 360.f);
    ImGui::InputText("Prefab", m_settings.prefab.data(), (int)m_settings.prefab.capacity()+1);

    if (ImGui::Button("Re-seed")) { EnsureSeed(); if (m_settings.shape==Shape::Circle) RebuildPreviewCircle(); else RebuildPreviewRect(); }
    ImGui::SameLine();
    if (ImGui::Button("Apply"))    { ApplyPlacement(*ctx.m_scene); }
    ImGui::SameLine();
    if (ImGui::Button("Clear"))    { m_preview.clear(); }

    ImGui::Separator();
    ImGui::Text("Preview points: %zu", m_preview.size());
  }
  ImGui::End();

  bool dirty = false;

int shape = (m_settings.shape == Shape::Circle ? 0 : 1);
ImGui::TextUnformatted("Shape");
dirty |= ImGui::RadioButton("Circle", &shape, 0); ImGui::SameLine();
dirty |= ImGui::RadioButton("Rect",   &shape, 1);
m_settings.shape = (shape == 0 ? Shape::Circle : Shape::Rect);

if (m_settings.shape == Shape::Circle) {
  dirty |= ImGui::SliderFloat("Radius (m)", &m_settings.radius, 1.f, 200.f);
} else {
  dirty |= ImGui::SliderFloat("Width (m)",  &m_settings.width,  1.f, 400.f);
  dirty |= ImGui::SliderFloat("Height (m)", &m_settings.height, 1.f, 400.f);
}

ImGui::Separator();
dirty |= ImGui::SliderFloat("Density (per m^2)", &m_settings.density, 0.0f, 1.0f);
dirty |= ImGui::SliderFloat("Min Spacing (m)",   &m_settings.minSpacing, 0.0f, 10.0f);
dirty |= ImGui::SliderFloat("Jitter [0..1]",     &m_settings.jitter, 0.0f, 1.0f);

ImGui::Separator();
dirty |= ImGui::InputInt  ("Seed", (int*)&m_settings.seed);
dirty |= ImGui::SliderFloat("Scale min", &m_settings.scaleMin, 0.2f, 5.0f);
dirty |= ImGui::SliderFloat("Scale max", &m_settings.scaleMax, 0.2f, 5.0f);
dirty |= ImGui::SliderFloat("Yaw min",   &m_settings.yawMinDeg, 0.f, 360.f);
dirty |= ImGui::SliderFloat("Yaw max",   &m_settings.yawMaxDeg, 0.f, 360.f);

static char prefabBuf[256] = {};
if (m_settings.prefab.size() && prefabBuf[0] == '\0') {
  std::strncpy(prefabBuf, m_settings.prefab.c_str(), sizeof(prefabBuf)-1);
}
if (ImGui::InputText("Prefab", prefabBuf, sizeof(prefabBuf))) {
  m_settings.prefab = prefabBuf;
  dirty = true;
}

if (ImGui::Button("Re-seed")) { EnsureSeed(); dirty = true; }
ImGui::SameLine();
if (ImGui::Button("Apply"))    { ApplyPlacement(*ctx.m_scene); }
ImGui::SameLine();
if (ImGui::Button("Clear"))    { m_preview.clear(); m_hasCenter = false; }

if (dirty && m_hasCenter) {
  // перестроить предпросмотр с тем же центром
  if (m_settings.shape == Shape::Circle) {
    m_anchor = m_center; m_current = m_center + glm::vec3(m_settings.radius,0,0);
    RebuildPreviewCircle();
  } else {
    m_anchor = m_center - glm::vec3(m_settings.width*0.5f,0,m_settings.height*0.5f);
    m_current= m_center + glm::vec3(m_settings.width*0.5f,0,m_settings.height*0.5f);
    RebuildPreviewRect();
  }
}
}

// ——— генераторы предпросмотра ———

void FoliageBrushTool::RebuildPreviewCircle() {
  m_preview.clear();
  const glm::vec2 c = { m_anchor.x, m_anchor.z };
  const glm::vec2 q = { m_current.x, m_current.z };
  const float R = (m_dragging ? glm::length(q - c) : m_settings.radius);

  // целевая площадь
  const float area = 3.14159265f * R * R;
  const float expectedN = area * m_settings.density;

  // простая «життер-сетка»: шаг базируется на minSpacing
  const float cell = std::max(m_settings.minSpacing, 0.5f);
  std::uniform_real_distribution<float> uni(-0.5f, 0.5f);

  for (float x = -R; x <= R; x += cell) {
    for (float y = -R; y <= R; y += cell) {
      if (x*x + y*y > R*R) continue;
      glm::vec2 p = { c.x + x, c.y + y };
      // життер в пределах ячейки
      p += glm::vec2(uni(m_rng), uni(m_rng)) * (cell * m_settings.jitter);
      m_preview.emplace_back(p.x, 0.f, p.y);
    }
  }
  // простая «разрежающая» фильтрация по minSpacing
  if (m_settings.minSpacing > 0.0f) {
    std::vector<glm::vec3> filtered;
    const float d2 = m_settings.minSpacing * m_settings.minSpacing;
    for (auto& p : m_preview) {
      bool ok = true;
      for (auto& f : filtered) {
        const float dx = p.x - f.x, dz = p.z - f.z;
        if (dx*dx + dz*dz < d2) { ok = false; break; }
      }
      if (ok) filtered.push_back(p);
    }
    m_preview.swap(filtered);
  }
  // ограничим в сторону expectedN (опционально)
  if (expectedN > 1 && m_preview.size() > (size_t)expectedN) {
    m_preview.resize((size_t)expectedN);
  }
}

void FoliageBrushTool::RebuildPreviewRect() {
  m_preview.clear();
  const glm::vec2 a = { m_anchor.x,  m_anchor.z };
  const glm::vec2 b = { m_current.x, m_current.z };
  const glm::vec2 minp = { std::min(a.x, b.x), std::min(a.y, b.y) };
  const glm::vec2 maxp = { std::max(a.x, b.x), std::max(a.y, b.y) };
  const glm::vec2 size = maxp - minp;

  const float area = size.x * size.y;
  const float expectedN = area * m_settings.density;

  const float cell = std::max(m_settings.minSpacing, 0.5f);
  std::uniform_real_distribution<float> uni(-0.5f, 0.5f);

  for (float x = minp.x; x <= maxp.x; x += cell) {
    for (float y = minp.y; y <= maxp.y; y += cell) {
      glm::vec2 p = { x, y };
      p += glm::vec2(uni(m_rng), uni(m_rng)) * (cell * m_settings.jitter);
      m_preview.emplace_back(p.x, 0.f, p.y);
    }
  }

  if (m_settings.minSpacing > 0.0f) {
    std::vector<glm::vec3> filtered;
    const float d2 = m_settings.minSpacing * m_settings.minSpacing;
    for (auto& p : m_preview) {
      bool ok = true;
      for (auto& f : filtered) {
        const float dx = p.x - f.x, dz = p.z - f.z;
        if (dx*dx + dz*dz < d2) { ok = false; break; }
      }
      if (ok) filtered.push_back(p);
    }
    m_preview.swap(filtered);
  }
  if (expectedN > 1 && m_preview.size() > (size_t)expectedN) {
    m_preview.resize((size_t)expectedN);
  }
}

// ——— размещение ———

void FoliageBrushTool::ApplyPlacement(engine::Scene& scene) {
  if (m_preview.empty()) return;

  std::uniform_real_distribution<float> unif01(0.f, 1.f);
  const float smin = m_settings.scaleMin, smax = m_settings.scaleMax;
  const float yawMin = glm::radians(m_settings.yawMinDeg);
  const float yawMax = glm::radians(m_settings.yawMaxDeg);

  for (auto& p : m_preview) {
    // scale (для «разной высоты» используешь масштаб по Y, но я даю униформ по всем осям)
    const float t = unif01(m_rng);
    const float s = smin + (smax - smin)*t;

    // поворот по Y (если модели поддерживают, можно хранить как часть SceneObject при расширении)
    (void)yawMin; (void)yawMax; // пока не сохраняем углы — при надобности расширь SceneObject

    // вызываем твой пайплайн:
    // сейчас Scene::AddTree(name=default_tree) создаёт Agent и SceneObject, скейл берётся из префаба
    // если хочешь именно свой s — либо расширь AddTree/SceneObject, либо применяй prefab.model_scale*s
    scene.AddTree(p.x, p.z, m_settings.prefab);
  }

  m_preview.clear();
}


// FoliageBrushTool.cc
bool FoliageBrushTool::ProjectToScreen(const EditorContext& ctx,
                                       const glm::vec3& p,
                                       glm::vec2& out_px) {
  const glm::vec4 clip = ctx.m_proj * ctx.m_view * glm::vec4(p, 1.0f);
  if (clip.w <= 0.0f) return false;
  const glm::vec3 ndc = glm::vec3(clip) / clip.w; // [-1..1]
  if (ndc.x < -1.f || ndc.x > 1.f || ndc.y < -1.f || ndc.y > 1.f) return false;
  // в пиксели framebuffer
  const float x = (ndc.x * 0.5f + 0.5f) * (float)ctx.vpW;
  const float y = (1.f - (ndc.y * 0.5f + 0.5f)) * (float)ctx.vpH;
  out_px = {x, y};
  return true;
}

void FoliageBrushTool::DrawOverlay(EditorContext& ctx) {
  auto* dl = ImGui::GetBackgroundDrawList(); // рисуем под UI (можно Foreground)
  const ImU32 colOutline = IM_COL32(80, 200, 120, 200);
  const ImU32 colPoints  = IM_COL32(80, 200, 120, 180);

  // контур области
  std::vector<ImVec2> poly;
  poly.reserve(128);

  if (m_hasCenter) {
    if (m_settings.shape == Shape::Circle) {
      const float R = (m_dragging
                       ? glm::length(glm::vec2(m_current.x-m_anchor.x, m_current.z-m_anchor.z))
                       : m_settings.radius);
      const int SEG = 64;
      for (int i=0;i<SEG;i++) {
        float a = (float)i / SEG * 2.f * 3.14159265f;
        glm::vec3 pw = m_center + glm::vec3(std::cos(a)*R, 0.f, std::sin(a)*R);
        glm::vec2 ps;
        if (ProjectToScreen(ctx, pw, ps)) poly.emplace_back(ps.x, ps.y);
      }
    } else { // Rect — по центру и размеру
      const glm::vec3 a = m_center + glm::vec3(-m_settings.width*0.5f, 0.f, -m_settings.height*0.5f);
      const glm::vec3 b = m_center + glm::vec3(+m_settings.width*0.5f, 0.f, -m_settings.height*0.5f);
      const glm::vec3 c = m_center + glm::vec3(+m_settings.width*0.5f, 0.f, +m_settings.height*0.5f);
      const glm::vec3 d = m_center + glm::vec3(-m_settings.width*0.5f, 0.f, +m_settings.height*0.5f);
      glm::vec2 s;
      if (ProjectToScreen(ctx, a, s)) poly.emplace_back(s.x,s.y);
      if (ProjectToScreen(ctx, b, s)) poly.emplace_back(s.x,s.y);
      if (ProjectToScreen(ctx, c, s)) poly.emplace_back(s.x,s.y);
      if (ProjectToScreen(ctx, d, s)) poly.emplace_back(s.x,s.y);
    }
  }

  if (poly.size() >= 2) {
    // замкнём
    for (size_t i=0;i<poly.size();++i) {
      const ImVec2 p0 = poly[i];
      const ImVec2 p1 = poly[(i+1)%poly.size()];
      dl->AddLine(p0, p1, colOutline, 2.0f);
    }
  }

  // точки предпросмотра
  for (const auto& p : m_preview) {
    glm::vec2 px;
    if (ProjectToScreen(ctx, p, px)) {
      dl->AddCircleFilled(ImVec2(px.x, px.y), 2.0f, colPoints, 8);
    }
  }
}


} // namespace ui
