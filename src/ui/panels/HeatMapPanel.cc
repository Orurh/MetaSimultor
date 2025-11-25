#include "HeatMapPanel.h"
#include "core/scene/Scene.h"
#include <imgui.h>

namespace ui {

void HeatMapPanel::Draw(EditorContext &ctx) {
  auto &heatMap = ctx.m_scene->GetHeatMap();

  if (!ImGui::Begin(Id(), nullptr)) {
    ImGui::End();
    return;
  }

  // --- переключатель режима: Temperature / Risk ---
  static int s_mode = 0; // 0 - Temperature, 1 - Risk
  ImGui::TextUnformatted("Mode:");
  ImGui::SameLine();
  ImGui::RadioButton("Temperature", &s_mode, 0);
  ImGui::SameLine();
  ImGui::RadioButton("Risk", &s_mode, 1);

  heatMap.SetMode(s_mode == 0 ? engine::HeatMapMode::Temperature
                              : engine::HeatMapMode::Risk);

  // Берём текстуру и статистику после выбора режима
  m_texture = heatMap.GetTexture();
  m_stats   = heatMap.GetStats();

  const float AMBIENT_K = m_stats.m_ambientT;
  const float AMBIENT_C = AMBIENT_K - 273.15F;
  const float MIN_C     = m_stats.m_minCellT - 273.15F;
  const float MAX_C     = m_stats.m_maxCellT - 273.15F;
  const float D_MIN_C   = MIN_C - AMBIENT_C;
  const float D_MAX_C   = MAX_C - AMBIENT_C;
  const float SPAN_C    = MAX_C - MIN_C;

  // Общая инфа по температуре
  ImGui::Text("Ambient:       %.1f K (%.1f C)", AMBIENT_K, AMBIENT_C);
  ImGui::Text("Cells range:   %.1f - %.1f K (%.1f - %.1f C)",
              m_stats.m_minCellT, m_stats.m_maxCellT, MIN_C, MAX_C);
  ImGui::Text("Delta to ambient: min %.1f C, max %.1f C", D_MIN_C, D_MAX_C);
  ImGui::Text("Span:          %.1f C", SPAN_C);
  ImGui::Text("Cooled points: %d",   m_stats.m_cooledPoints);
  ImGui::Text("Burned agents: %d",   m_stats.m_burnedAgents);
  ImGui::Separator();

  // Блок "температурного риска" — как раньше
  ImGui::TextUnformatted("Temperature-based risk (draft):");
  ImGui::BulletText("Mean ΔT: %.1f C", m_stats.m_meanDeltaC);
  ImGui::BulletText("Max  ΔT: %.1f C", m_stats.m_maxDeltaC);
  ImGui::BulletText("Warm area (ΔT > 20 C):   %.1f %%",
                    m_stats.m_warmArea01 * 100.0F);
  ImGui::BulletText("Critical area (ΔT > 150 C): %.1f %%",
                    m_stats.m_criticalArea01 * 100.0F);
  ImGui::Separator();

  // диапазоны должны быть согласованы с HeatMap::UploadTemperatureToTexture
  constexpr float COLD_RANGE_C = 20.0F;
  constexpr float WARM_RANGE_C = 60.0F;
  constexpr float FIRE_RANGE_C = 800.0F; // просто для текста

  if (s_mode == 0) {
    ImGui::TextUnformatted("Zones by ΔT (C):");
    ImGui::BulletText("Cold:   T < ambient - %.0f C", COLD_RANGE_C);
    ImGui::BulletText("Warm:   ambient .. ambient + %.0f C", WARM_RANGE_C);
    ImGui::BulletText("Hot:    ambient + %.0f C .. ambient + %.0f C",
                      WARM_RANGE_C, WARM_RANGE_C + FIRE_RANGE_C);
    ImGui::BulletText("Fire core: >> ambient + %.0f C", WARM_RANGE_C);
  } else {
    ImGui::TextUnformatted("Risk scale (heuristic, 0..1):");
    ImGui::BulletText("Low:        ~0.0 .. 0.3");
    ImGui::BulletText("Medium:     ~0.3 .. 0.6");
    ImGui::BulletText("High:       ~0.6 .. 0.9");
    ImGui::BulletText("Critical:   ~0.9 .. 1.0");
  }
  ImGui::Separator();

  if (m_texture) {
    static float s_displayScale = 0.5F;
    ImGui::SliderFloat("Scale", &s_displayScale, 0.1f, 2.0f);

    uint32_t texWidth  = m_texture->GetWidth();
    uint32_t texHeight = m_texture->GetHeight();
    ImVec2   imageSize(texWidth * s_displayScale,
                       texHeight * s_displayScale);

    auto textureID = static_cast<ImTextureID>(
        static_cast<intptr_t>(m_texture->GetRendererID()));
    ImGui::Image(textureID, imageSize);

    // Легенда
    ImGui::Spacing();
    if (s_mode == 0) {
      ImGui::TextUnformatted("Legend (temperature):");
      ImGui::SameLine();
      ImGui::ColorButton("##cold",    ImVec4(0.0F, 0.2F, 0.8F, 1.0F)); ImGui::SameLine();
      ImGui::TextUnformatted("cold");
      ImGui::SameLine();
      ImGui::ColorButton("##neutral", ImVec4(0.0F, 0.8F, 0.0F, 1.0F)); ImGui::SameLine();
      ImGui::TextUnformatted("warm");
      ImGui::SameLine();
      ImGui::ColorButton("##hot",     ImVec4(1.0F, 0.3F, 0.0F, 1.0F)); ImGui::SameLine();
      ImGui::TextUnformatted("hot / fire");
    } else {
      ImGui::TextUnformatted("Legend (risk):");
      ImGui::SameLine();
      ImGui::ColorButton("##risk_low",
                         ImVec4(0.0F, 0.4F, 0.0F, 1.0F)); ImGui::SameLine();
      ImGui::TextUnformatted("low");
      ImGui::SameLine();
      ImGui::ColorButton("##risk_med",
                         ImVec4(0.8F, 0.8F, 0.0F, 1.0F)); ImGui::SameLine();
      ImGui::TextUnformatted("medium");
      ImGui::SameLine();
      ImGui::ColorButton("##risk_high",
                         ImVec4(1.0F, 0.5F, 0.0F, 1.0F)); ImGui::SameLine();
      ImGui::TextUnformatted("high");
      ImGui::SameLine();
      ImGui::ColorButton("##risk_crit",
                         ImVec4(1.0F, 0.0F, 0.0F, 1.0F)); ImGui::SameLine();
      ImGui::TextUnformatted("critical");
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup |
                             ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
      ImVec2 mousePos = ImGui::GetMousePos();
      ImVec2 itemMin  = ImGui::GetItemRectMin();
      ImVec2 itemMax  = ImGui::GetItemRectMax();

      if (mousePos.x >= itemMin.x && mousePos.x <= itemMax.x &&
          mousePos.y >= itemMin.y && mousePos.y <= itemMax.y) {

        float uvX = (mousePos.x - itemMin.x) / (itemMax.x - itemMin.x);
        float uvY = (mousePos.y - itemMin.y) / (itemMax.y - itemMin.y);

        ImGui::SetTooltip("UV: (%.2f, %.2f)", uvX, uvY);
      }
    }
  }

  ImGui::End();
}

} // namespace ui
