#include "HeatMapPanel.h"
#include "core/scene/Scene.h"
#include <imgui.h>

namespace ui {
void HeatMapPanel::Draw(EditorContext &ctx) {
  decltype(auto) heatMap = ctx.m_scene->GetHeatMap();
  m_texture = heatMap.GetTexture();
  m_stats = heatMap.GetStats();

  if (!ImGui::Begin(Id(), nullptr)) {
    ImGui::End();
    return;
  }

  ImGui::Text("Temperature Range: %.1f - %.1f", m_stats.minT, m_stats.maxT);
  ImGui::Separator();

  if (m_texture) {
    static float displayScale = 0.5f;
    ImGui::SliderFloat("Scale", &displayScale, 0.1f, 2.0f);

    uint32_t texWidth = m_texture->GetWidth();
    uint32_t texHeight = m_texture->GetHeight();
    ImVec2 imageSize(texWidth * displayScale, texHeight * displayScale);

    ImTextureID textureID = (ImTextureID)(intptr_t)m_texture->GetRendererID();
    ImGui::Image(textureID, imageSize);

    if (ImGui::IsItemHovered()) {
      ImVec2 mousePos = ImGui::GetMousePos();
      ImVec2 itemMin = ImGui::GetItemRectMin();
      ImVec2 itemMax = ImGui::GetItemRectMax();

      if (mousePos.x >= itemMin.x && mousePos.x <= itemMax.x && mousePos.y >= itemMin.y && mousePos.y <= itemMax.y) {

        float uvX = (mousePos.x - itemMin.x) / (itemMax.x - itemMin.x);
        float uvY = (mousePos.y - itemMin.y) / (itemMax.y - itemMin.y);

        ImGui::SetTooltip("UV: (%.2f, %.2f)", uvX, uvY);
      }
    }
  }

  ImGui::End();
}

}  // namespace ui