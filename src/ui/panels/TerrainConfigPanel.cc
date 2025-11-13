// src/ui/panels/TerrainConfigPanel.cc
#include "TerrainConfigPanel.h"
#include "core/scene/Scene.h"
#include "core/scene/Terrain.h"
#include <filesystem>
#include <imgui.h>
#include <random>

namespace ui {

void TerrainConfigPanel::Draw(EditorContext &ctx) {
  ImGui::Begin("Terrain Configuration", nullptr,
               ImGuiWindowFlags_AlwaysAutoResize);

  ImGui::PushItemWidth(120);
  const auto *terrain = ctx.m_scene ? ctx.m_scene->GetTerrain() : nullptr;
  if (ImGui::IsWindowAppearing() || ctx.m_sceneChanged) {
    m_settings.width = terrain->GetWidth();
    m_settings.depth = terrain->GetDepth();
    m_settings.scale = terrain->GetScale();
    m_settings.maxHeight = terrain->GetMaxHeight();
    m_settings.seed = terrain->GetSeed();
    m_settings.frequency = terrain->GetFrequency();
    m_settings.octaves = terrain->GetOctaves();
    m_settings.persistence = terrain->GetPersistence();
    m_settings.lacunarity = terrain->GetLacunarity();
    m_settings.position = terrain->GetPosition();
    m_settings.textureSettings = terrain->GetTextureSettings();

    // Инициализируем буфер пути текстуры
    strncpy(m_texturePath, m_settings.textureSettings.diffuseMap.c_str(),
            sizeof(m_texturePath) - 1);
    m_texturePath[sizeof(m_texturePath) - 1] = '\0';

    // Загружаем превью если текстура уже есть
    if (!m_settings.textureSettings.diffuseMap.empty()) {
      LoadAndPreviewTexture(m_settings.textureSettings.diffuseMap);
    }

    ctx.m_sceneChanged = false;
  }
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
  ImGui::SeparatorText("Position");

  bool pos_changed = false;
  pos_changed |=
      ImGui::SliderFloat("X", &m_settings.position.x, -500.0f, 500.0f, "%.1f");
  pos_changed |=
      ImGui::SliderFloat("Y", &m_settings.position.y, -200.0f, 200.0f, "%.1f");
  pos_changed |=
      ImGui::SliderFloat("Z", &m_settings.position.z, -500.0f, 500.0f, "%.1f");

  if (pos_changed) {
    ctx.m_scene->GetTerrain()->SetPosition(m_settings.position);
  }

  DrawTextureSettings(ctx);

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

  if (m_regenerate || changed || m_textureChanged) {
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
    s.textureSettings = m_settings.textureSettings;

    ctx.m_scene->GenerateTerrain(s);
    m_regenerate = false;
    m_textureChanged = false;
  }
  ImGui::End();
}

void TerrainConfigPanel::LoadAndPreviewTexture(const std::string &path) {
  try {
    if (std::filesystem::exists(path)) {
      m_texturePreview = engine::Texture::Create(path);
      m_textureLoaded = true;
      strncpy(m_texturePath, path.c_str(), sizeof(m_texturePath) - 1);
      m_texturePath[sizeof(m_texturePath) - 1] = '\0';
    } else {
      m_textureLoaded = false;
      m_texturePreview.reset();
    }
  } catch (const std::exception &e) {
    m_textureLoaded = false;
    m_texturePreview.reset();
  }
}

void TerrainConfigPanel::DrawTexturePreview() {
  if (!m_textureLoaded || !m_texturePreview) {
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "texture not loaded");
    return;
  }

  auto texture_id = m_texturePreview->GetId();

  if (texture_id) {
    float preview_size = 100.0f;

    ImGui::BeginChild((std::string("TexturePreview")).c_str(),
                      ImVec2(preview_size + 10, preview_size + 30), true);
    {
      ImGui::Image((ImTextureID)(intptr_t)texture_id,
                   ImVec2(preview_size, preview_size));
      ImGui::Text("Size: %dx%d", m_texturePreview->GetWidth(),
                  m_texturePreview->GetHeight());
    }
    ImGui::EndChild();
  }
}

void TerrainConfigPanel::DrawTextureSettings(EditorContext &ctx) {
  ImGui::SeparatorText("Texture");
  ImGui::PushItemWidth(300);
  // Поле для ввода пути к текстуре
  if (ImGui::InputText("Texture Path", m_texturePath, sizeof(m_texturePath))) {
    // Путь был изменен, но не применяем сразу
  }

  // Кнопка применения текстуры
  ImGui::SameLine();
  if (ImGui::Button("Apply")) {
    m_settings.textureSettings.diffuseMap = m_texturePath;
    LoadAndPreviewTexture(m_texturePath);
    m_textureChanged = true;
  }

  // Отображение текущего пути
  if (!m_settings.textureSettings.diffuseMap.empty()) {
    ImGui::Text("Current: %s", m_settings.textureSettings.diffuseMap.c_str());

    // Кнопка удаления текстуры
    ImGui::SameLine();
    if (ImGui::Button("Remove")) {
      m_settings.textureSettings.diffuseMap.clear();
      memset(m_texturePath, 0, sizeof(m_texturePath));
      m_textureLoaded = false;
      m_texturePreview.reset();
      m_textureChanged = true;
    }
  } else {
    ImGui::Text("No texture loaded");
  }
  DrawTexturePreview();
  // Настройки масштаба текстуры
  if (ImGui::SliderFloat("Texture Scale",
                         &m_settings.textureSettings.textureScale, 1.0f, 100.0f,
                         "%.1f")) {
    m_textureChanged = true;
  }

  // Настройки материала
  ImGui::SeparatorText("Material");
  if (ImGui::ColorEdit3("Specular", &m_settings.textureSettings.specular.x)) {
    m_textureChanged = true;
  }
  if (ImGui::SliderFloat("Shininess", &m_settings.textureSettings.shininess,
                         1.0f, 128.0f, "%.0f")) {
    m_textureChanged = true;
  }
}

} // namespace ui