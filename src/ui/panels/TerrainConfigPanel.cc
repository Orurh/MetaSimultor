// src/ui/panels/TerrainConfigPanel.cc
#include "TerrainConfigPanel.h"
#include "core/scene/Scene.h"
#include "core/scene/Terrain.h"
#include <imgui.h>
#include <random>
namespace ui {

void TerrainConfigPanel::Draw(EditorContext &ctx) {
  ImGui::Begin("Terrain Configuration", nullptr,
               ImGuiWindowFlags_AlwaysAutoResize);

  ImGui::PushItemWidth(120);

  bool changed = false;

  changed |= ImGui::SliderInt("Width", &m_settings.width, 32, 512, "%d",
                              ImGuiSliderFlags_AlwaysClamp);
  changed |= ImGui::SliderInt("Depth", &m_settings.depth, 32, 512, "%d",
                              ImGuiSliderFlags_AlwaysClamp);
  changed |=
      ImGui::SliderFloat("Scale", &m_settings.scale, 0.1f, 10.0f, "%.2f");
  changed |= ImGui::SliderFloat("Max Height", &m_settings.maxHeight, 1.0f,
                                300.0f, "%.1f");
  changed |= ImGui::SliderInt("Octaves", &m_settings.octaves, 1, 10);
  changed |= ImGui::SliderFloat("Frequency", &m_settings.frequency, 0.001f,
                                0.1f, "%.4f");
  changed |= ImGui::SliderFloat("Persistence", &m_settings.persistence, 0.1f,
                                1.0f, "%.2f");
  changed |= ImGui::SliderFloat("Lacunarity", &m_settings.lacunarity, 1.0f,
                                4.0f, "%.2f");

  ImGui::PopItemWidth();

  ImGui::Separator();

  if (ImGui::Button("Random Seed")) {
    std::random_device rd;
    m_settings.seed = rd();
    changed = true;
  }
  ImGui::SameLine();
  ImGui::Text("Seed: %u", m_settings.seed);
  changed |=
      ImGui::InputInt("##seed", reinterpret_cast<int *>(&m_settings.seed));

  ImGui::Separator();

  if (ImGui::Button("Regenerate Terrain", ImGui::GetContentRegionAvail())) {
    m_regenerate = true;
  }

  if (m_regenerate || changed) {
    engine::TerrainGenerationSettings s;
    s.width = m_settings.width;
    s.depth = m_settings.depth;
    s.scale = m_settings.scale;
    s.maxHeight = m_settings.maxHeight;
    s.seed = m_settings.seed;
    s.frequency = m_settings.frequency;
    s.octaves = m_settings.octaves;
    s.persistence = m_settings.persistence;
    s.lacunarity = m_settings.lacunarity;
    ctx.m_scene->GenerateTerrain(s);
    m_regenerate = false;
  }
  ImGui::End();
}

} // namespace ui