#include "FireParamsPanel.h"
#include "graphics/RenderPolicy/OpenGLPolicy.h"
#include "graphics/Renderer.h"
#include "ui/EditorContext.h"
#include <fstream>
#include <imgui.h>
#include <iostream>
#include <yaml-cpp/yaml.h>

namespace ui {

void FireParamsPanel::Draw(EditorContext &ctx) {
  if (!ImGui::Begin(Id())) {
    ImGui::End();
    return;
  }

  DrawPresetsSection();

  ImGui::Separator();

  // Кнопки управления
  if (ImGui::Button("Reset to Default")) {
    m_params = FireParams();
    ApplyParamsToShader();
  }
  ImGui::SameLine();
  if (ImGui::Button("Enable All")) {
    m_params.fire.enabled = true;
    m_params.smoke.enabled = true;
    m_params.sparks.enabled = true;
    m_params.ash.enabled = true;
    m_params.heat.enabled = true;
    ApplyParamsToShader();
  }
  ImGui::SameLine();
  if (ImGui::Button("Disable All")) {
    m_params.fire.enabled = false;
    m_params.smoke.enabled = false;
    m_params.sparks.enabled = false;
    m_params.ash.enabled = false;
    m_params.heat.enabled = false;
    ApplyParamsToShader();
  }

  ImGui::Separator();

  // ВКЛАДКИ
  if (ImGui::BeginTabBar("EffectsTabs")) {
    // Глобальные настройки
    if (ImGui::BeginTabItem("Global")) {
      m_currentTab = 0;
      DrawGlobalTab();
      ImGui::EndTabItem();
    }

    // Настройки эффектов
    if (ImGui::BeginTabItem("Fire")) {
      m_currentTab = 1;
      DrawFireTab();
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Smoke")) {
      m_currentTab = 2;
      DrawSmokeTab();
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Sparks")) {
      m_currentTab = 3;
      DrawSparksTab();
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Ash")) {
      m_currentTab = 4;
      DrawAshTab();
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Heat")) {
      m_currentTab = 5;
      DrawHeatTab();
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Textures")) {
      m_currentTab = 6;
      DrawTexturesTab();
      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }

  // Дебаунсинг обновлений
  auto now = std::chrono::steady_clock::now();
  if (m_needsUpdate && (now - m_lastUpdateTime) > UPDATE_DELAY) {
    ApplyParamsToShader();
    m_needsUpdate = false;
  }

  ImGui::End();
}

void FireParamsPanel::DrawGlobalTab() {
  ImGui::SeparatorText("Global Settings");

  bool changed = false;

  ImGui::SeparatorText("Quick Toggles");

  ImGui::BeginTable("QuickToggles", 5, ImGuiTableFlags_SizingFixedFit);
  ImGui::TableNextRow();

  ImGui::TableSetColumnIndex(0);
  changed |= ImGui::Checkbox("Fire", &m_params.fire.enabled);

  ImGui::TableSetColumnIndex(1);
  changed |= ImGui::Checkbox("Smoke", &m_params.smoke.enabled);

  ImGui::TableSetColumnIndex(2);
  changed |= ImGui::Checkbox("Sparks", &m_params.sparks.enabled);

  ImGui::TableSetColumnIndex(3);
  changed |= ImGui::Checkbox("Ash", &m_params.ash.enabled);

  ImGui::TableSetColumnIndex(4);
  changed |= ImGui::Checkbox("Heat", &m_params.heat.enabled);

  ImGui::EndTable();

  if (changed) {
    m_needsUpdate = true;
    m_lastUpdateTime = std::chrono::steady_clock::now();
  }
}

void FireParamsPanel::DrawEffectParams(const char *name, FireParams::EffectParams &params, int effectType,
                                       bool showColor) {
  ImGui::SeparatorText(name);

  bool changed = false;

  changed |= ImGui::Checkbox("Enabled", &params.enabled);

  if (params.enabled) {
    ImGui::Indent();

    ImGui::Text("Size & Animation");
    changed |= ImGui::SliderFloat("Particle Size", &params.particleSize, 0.1f, 10.0f);
    changed |= ImGui::SliderFloat("Size Variation", &params.sizeVariation, 0.0f, 3.0f);
    changed |= ImGui::SliderFloat("Animation Speed", &params.animationSpeed, 0.1f, 5.0f);
    changed |= ImGui::SliderFloat("Lifetime", &params.lifetime, 0.5f, 15.0f);
    changed |= ImGui::SliderFloat("Spawn Rate", &params.spawnRate, 1.0f, 500.0f);

    ImGui::Text("Movement");
    changed |= ImGui::SliderFloat("Rise Speed", &params.riseSpeed, 0.0f, 10.0f);
    changed |= ImGui::SliderFloat("Turbulence", &params.turbulence, 0.0f, 3.0f);
    changed |= ImGui::SliderFloat("Wind Influence", &params.windInfluence, 0.0f, 3.0f);
    changed |= ImGui::SliderFloat("Spread", &params.spread, 0.1f, 5.0f);

    if (showColor) {
      ImGui::Text("Colors");
      changed |= ImGui::ColorEdit3("Base Color", &params.baseColor[0]);
      changed |= ImGui::ColorEdit3("End Color", &params.endColor[0]);
      changed |= ImGui::SliderFloat("Color Variation", &params.colorVariation, 0.0f, 1.0f);
    }
    ImGui::Text("Color Effects");
    changed |= ImGui::SliderFloat("Red Intensity", &params.redIntensity, 0.0f, 2.0f);
    changed |= ImGui::SliderFloat("Yellow Reduction", &params.yellowReduction, 0.0f, 1.0f);
    changed |= ImGui::SliderFloat("Alpha Multiplier", &params.alphaMultiplier, 0.1f, 3.0f);
    changed |= ImGui::SliderFloat("Texture Influence", &params.textureInfluence, 0.0f, 1.0f);

    // Информация о эффекте
    ImGui::TextDisabled("Effect appears at: ");
    if (effectType == 0)
      ImGui::TextDisabled("Always when burning");
    else if (effectType == 1)
      ImGui::TextDisabled("Medium fire intensity");
    else if (effectType == 2)
      ImGui::TextDisabled("High fire intensity");
    else if (effectType == 3)
      ImGui::TextDisabled("Any fire intensity");
    else if (effectType == 4)
      ImGui::TextDisabled("Creates heat distortion");

    ImGui::Unindent();
  }

  if (changed) {
    m_needsUpdate = true;
    m_lastUpdateTime = std::chrono::steady_clock::now();
  }
}

void FireParamsPanel::DrawFireTab() { DrawEffectParams("Fire Parameters", m_params.fire, 0, true); }

void FireParamsPanel::DrawSmokeTab() { DrawEffectParams("Smoke Parameters", m_params.smoke, 1, true); }

void FireParamsPanel::DrawSparksTab() { DrawEffectParams("Sparks Parameters", m_params.sparks, 2, true); }

void FireParamsPanel::DrawAshTab() { DrawEffectParams("Ash Parameters", m_params.ash, 3, false); }

void FireParamsPanel::DrawHeatTab() { DrawEffectParams("Heat Parameters", m_params.heat, 4, false); }

void FireParamsPanel::DrawTexturesTab() {
  ImGui::SeparatorText("Effect Textures");

  bool changed = false;

  // Огонь
  ImGui::Text("Fire Texture:");
  if (ImGui::InputText("##FireTex", m_texturePaths[0], sizeof(m_texturePaths[0]))) {
    m_params.fireTexturePath = m_texturePaths[0];
    LoadAndPreviewTexture(m_params.fireTexturePath, 0);
    changed = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Load##Fire")) {
    LoadAndPreviewTexture(m_params.fireTexturePath, 0);
    changed = true;
  }
  DrawTexturePreview(0, "Fire");

  // Дым
  ImGui::Text("Smoke Texture:");
  if (ImGui::InputText("##SmokeTex", m_texturePaths[1], sizeof(m_texturePaths[1]))) {
    m_params.smokeTexturePath = m_texturePaths[1];
    LoadAndPreviewTexture(m_params.smokeTexturePath, 1);
    changed = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Load##Smoke")) {
    LoadAndPreviewTexture(m_params.smokeTexturePath, 1);
    changed = true;
  }
  DrawTexturePreview(1, "Smoke");

  // Искры
  ImGui::Text("Sparks Texture:");
  if (ImGui::InputText("##SparksTex", m_texturePaths[2], sizeof(m_texturePaths[2]))) {
    m_params.sparksTexturePath = m_texturePaths[2];
    LoadAndPreviewTexture(m_params.sparksTexturePath, 2);
    changed = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Load##Sparks")) {
    LoadAndPreviewTexture(m_params.sparksTexturePath, 2);
    changed = true;
  }
  DrawTexturePreview(2, "Sparks");

  // Пепел
  ImGui::Text("Ash Texture:");
  if (ImGui::InputText("##AshTex", m_texturePaths[3], sizeof(m_texturePaths[3]))) {
    m_params.ashTexturePath = m_texturePaths[3];
    LoadAndPreviewTexture(m_params.ashTexturePath, 3);
    changed = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Load##Ash")) {
    LoadAndPreviewTexture(m_params.ashTexturePath, 3);
    changed = true;
  }
  DrawTexturePreview(3, "Ash");

  // Тепло
  ImGui::Text("Heat Texture:");
  if (ImGui::InputText("##HeatTex", m_texturePaths[4], sizeof(m_texturePaths[4]))) {
    m_params.heatTexturePath = m_texturePaths[4];
    LoadAndPreviewTexture(m_params.heatTexturePath, 4);
    changed = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Load##Heat")) {
    LoadAndPreviewTexture(m_params.heatTexturePath, 4);
    changed = true;
  }
  DrawTexturePreview(4, "Heat");

  if (changed) {
    m_needsUpdate = true;
    m_lastUpdateTime = std::chrono::steady_clock::now();
  }
}

void FireParamsPanel::LoadAndPreviewTexture(const std::string &path, int textureIndex) {
  try {
    if (std::filesystem::exists(path)) {
      m_texturePreviews[textureIndex] = engine::Texture::Create(path);
      m_textureLoaded[textureIndex] = true;
      strncpy(m_texturePaths[textureIndex], path.c_str(), sizeof(m_texturePaths[textureIndex]) - 1);
      std::cout << "Texture preview loaded: " << path << std::endl;
    } else {
      m_textureLoaded[textureIndex] = false;
      std::cout << "Texture file not found: " << path << std::endl;
    }
  } catch (const std::exception &e) {
    m_textureLoaded[textureIndex] = false;
    std::cerr << "Failed to load texture: " << e.what() << std::endl;
  }
}

void FireParamsPanel::DrawTexturePreview(int textureIndex, const char *name) {
  if (!m_textureLoaded[textureIndex] || !m_texturePreviews[textureIndex]) {
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s texture not loaded", name);
    return;
  }

  auto texture_id = m_texturePreviews[textureIndex]->GetId();

  if (texture_id) {
    float preview_size = 100.0f;

    ImGui::BeginChild((std::string("TexturePreview") + name).c_str(), ImVec2(preview_size + 10, preview_size + 30),
                      true);
    {
      ImGui::Image((ImTextureID)(intptr_t)texture_id, ImVec2(preview_size, preview_size));
      ImGui::Text("Size: %dx%d", m_texturePreviews[textureIndex]->GetWidth(),
                  m_texturePreviews[textureIndex]->GetHeight());
    }
    ImGui::EndChild();
  }
}

void FireParamsPanel::DrawPresetsSection() {
  ImGui::SeparatorText("Presets");

  // Создание директории для пресетов, если её нет
  if (!std::filesystem::exists(m_presetsDirectory)) {
    std::filesystem::create_directory(m_presetsDirectory);
  }

  // Кнопка обновления списка пресетов
  if (ImGui::Button("Refresh Presets")) {
    RefreshPresetsList();
  }
  ImGui::SameLine();

  // Сохранение пресета
  ImGui::InputText("##NewPresetName", m_newPresetName, sizeof(m_newPresetName));
  ImGui::SameLine();
  if (ImGui::Button("Save Preset") && strlen(m_newPresetName) > 0) {
    SavePreset(m_newPresetName);
    m_newPresetName[0] = '\0';
    RefreshPresetsList();
  }

  // Список пресетов
  if (!m_presets.empty()) {
    ImGui::Text("Available Presets:");
    ImGui::BeginChild("PresetsList", ImVec2(0, 150), true);

    for (const auto &[name, preset] : m_presets) {
      ImGui::PushID(name.c_str());

      bool isCurrent = (m_currentPresetName == name);
      if (ImGui::Button("Load")) {
        LoadPreset(name);
      }
      ImGui::SameLine();

      if (ImGui::Button("Delete")) {
        DeletePreset(name);
        ImGui::PopID();
        break;
      }
      ImGui::SameLine();

      if (isCurrent) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", name.c_str());
      } else {
        ImGui::Text("%s", name.c_str());
      }

      ImGui::PopID();
    }

    ImGui::EndChild();
  } else {
    ImGui::TextDisabled("No presets found. Create one or refresh the list.");
  }
}

void FireParamsPanel::SavePreset(const std::string &name) {
  Preset preset;
  preset.name = name;
  preset.params = m_params;

  SavePresetToYAML(preset);
  m_currentPresetName = name;

  std::cout << "Preset saved: " << name << std::endl;
}

void FireParamsPanel::LoadPreset(const std::string &name) {
  std::string filename = m_presetsDirectory + "/" + name + ".yaml";
  Preset preset;

  if (LoadPresetFromYAML(filename, preset)) {
    m_params = preset.params;
    m_currentPresetName = name;

    // Обновляем пути текстур в UI
    strncpy(m_texturePaths[0], m_params.fireTexturePath.c_str(), sizeof(m_texturePaths[0]) - 1);
    strncpy(m_texturePaths[1], m_params.smokeTexturePath.c_str(), sizeof(m_texturePaths[1]) - 1);
    strncpy(m_texturePaths[2], m_params.sparksTexturePath.c_str(), sizeof(m_texturePaths[2]) - 1);
    strncpy(m_texturePaths[3], m_params.ashTexturePath.c_str(), sizeof(m_texturePaths[3]) - 1);
    strncpy(m_texturePaths[4], m_params.heatTexturePath.c_str(), sizeof(m_texturePaths[4]) - 1);

    // Перезагружаем preview текстур
    for (int i = 0; i < 5; i++) {
      LoadAndPreviewTexture(m_texturePaths[i], i);
    }

    ApplyParamsToShader();

    std::cout << "Preset loaded: " << name << std::endl;
  }
}

void FireParamsPanel::DeletePreset(const std::string &name) {
  std::string filename = m_presetsDirectory + "/" + name + ".yaml";
  if (std::filesystem::exists(filename)) {
    std::filesystem::remove(filename);
    m_presets.erase(name);
    if (m_currentPresetName == name) {
      m_currentPresetName.clear();
    }
    std::cout << "Preset deleted: " << name << std::endl;
  }
}

void FireParamsPanel::RefreshPresetsList() {
  m_presets.clear();

  if (!std::filesystem::exists(m_presetsDirectory)) {
    return;
  }

  for (const auto &entry : std::filesystem::directory_iterator(m_presetsDirectory)) {
    if (entry.path().extension() == ".yaml") {
      std::string filename = entry.path().stem().string();
      Preset preset;

      if (LoadPresetFromYAML(entry.path().string(), preset)) {
        m_presets[filename] = preset;
      }
    }
  }

  std::cout << "Refreshed presets list. Found: " << m_presets.size() << " presets" << std::endl;
}

void FireParamsPanel::SavePresetToYAML(const Preset &preset) {
  YAML::Emitter out;

  out << YAML::BeginMap;
  out << YAML::Key << "name" << YAML::Value << preset.name;

  // Global settings
  // Texture paths
  out << YAML::Key << "fireTexturePath" << YAML::Value << preset.params.fireTexturePath;
  out << YAML::Key << "smokeTexturePath" << YAML::Value << preset.params.smokeTexturePath;
  out << YAML::Key << "sparksTexturePath" << YAML::Value << preset.params.sparksTexturePath;
  out << YAML::Key << "ashTexturePath" << YAML::Value << preset.params.ashTexturePath;
  out << YAML::Key << "heatTexturePath" << YAML::Value << preset.params.heatTexturePath;

  // Effect parameters
  auto saveEffectParams = [&](const std::string &effectName, const FireParams::EffectParams &params) {
    out << YAML::Key << effectName + "_enabled" << YAML::Value << params.enabled;
    out << YAML::Key << effectName + "_particleSize" << YAML::Value << params.particleSize;
    out << YAML::Key << effectName + "_sizeVariation" << YAML::Value << params.sizeVariation;
    out << YAML::Key << effectName + "_animationSpeed" << YAML::Value << params.animationSpeed;
    out << YAML::Key << effectName + "_lifetime" << YAML::Value << params.lifetime;
    out << YAML::Key << effectName + "_spawnRate" << YAML::Value << params.spawnRate;
    out << YAML::Key << effectName + "_riseSpeed" << YAML::Value << params.riseSpeed;
    out << YAML::Key << effectName + "_turbulence" << YAML::Value << params.turbulence;
    out << YAML::Key << effectName + "_windInfluence" << YAML::Value << params.windInfluence;
    out << YAML::Key << effectName + "_spread" << YAML::Value << params.spread;
    out << YAML::Key << effectName + "_colorVariation" << YAML::Value << params.colorVariation;

    out << YAML::Key << effectName + "_redIntensity" << YAML::Value << params.redIntensity;
    out << YAML::Key << effectName + "_yellowReduction" << YAML::Value << params.yellowReduction;
    out << YAML::Key << effectName + "_alphaMultiplier" << YAML::Value << params.alphaMultiplier;
    out << YAML::Key << effectName + "_textureInfluence" << YAML::Value << params.textureInfluence;

    out << YAML::Key << effectName + "_baseColor" << YAML::Value << YAML::Flow << YAML::BeginSeq << params.baseColor[0]
        << params.baseColor[1] << params.baseColor[2] << YAML::EndSeq;
    out << YAML::Key << effectName + "_endColor" << YAML::Value << YAML::Flow << YAML::BeginSeq << params.endColor[0]
        << params.endColor[1] << params.endColor[2] << YAML::EndSeq;
  };

  saveEffectParams("fire", preset.params.fire);
  saveEffectParams("smoke", preset.params.smoke);
  saveEffectParams("sparks", preset.params.sparks);
  saveEffectParams("ash", preset.params.ash);
  saveEffectParams("heat", preset.params.heat);

  out << YAML::EndMap;

  std::string filename = m_presetsDirectory + "/" + preset.name + ".yaml";
  std::ofstream file(filename);
  file << out.c_str();
}

bool FireParamsPanel::LoadPresetFromYAML(const std::string &filename, Preset &preset) {
  try {
    YAML::Node config = YAML::LoadFile(filename);

    preset.name = config["name"].as<std::string>();

    // Global settings

    // Texture paths
    if (config["fireTexturePath"])
      preset.params.fireTexturePath = config["fireTexturePath"].as<std::string>();
    if (config["smokeTexturePath"])
      preset.params.smokeTexturePath = config["smokeTexturePath"].as<std::string>();
    if (config["sparksTexturePath"])
      preset.params.sparksTexturePath = config["sparksTexturePath"].as<std::string>();
    if (config["ashTexturePath"])
      preset.params.ashTexturePath = config["ashTexturePath"].as<std::string>();
    if (config["heatTexturePath"])
      preset.params.heatTexturePath = config["heatTexturePath"].as<std::string>();

    // Effect parameters loader
    auto loadEffectParams = [&](const std::string &effectName, FireParams::EffectParams &params) {
      if (config[effectName + "_enabled"])
        params.enabled = config[effectName + "_enabled"].as<bool>();
      if (config[effectName + "_particleSize"])
        params.particleSize = config[effectName + "_particleSize"].as<float>();
      if (config[effectName + "_sizeVariation"])
        params.sizeVariation = config[effectName + "_sizeVariation"].as<float>();
      if (config[effectName + "_animationSpeed"])
        params.animationSpeed = config[effectName + "_animationSpeed"].as<float>();
      if (config[effectName + "_lifetime"])
        params.lifetime = config[effectName + "_lifetime"].as<float>();
      if (config[effectName + "_spawnRate"])
        params.spawnRate = config[effectName + "_spawnRate"].as<float>();
      if (config[effectName + "_riseSpeed"])
        params.riseSpeed = config[effectName + "_riseSpeed"].as<float>();
      if (config[effectName + "_turbulence"])
        params.turbulence = config[effectName + "_turbulence"].as<float>();
      if (config[effectName + "_windInfluence"])
        params.windInfluence = config[effectName + "_windInfluence"].as<float>();
      if (config[effectName + "_spread"])
        params.spread = config[effectName + "_spread"].as<float>();
      if (config[effectName + "_colorVariation"])
        params.colorVariation = config[effectName + "_colorVariation"].as<float>();

      if (config[effectName + "_redIntensity"])
        params.redIntensity = config[effectName + "_redIntensity"].as<float>();
      if (config[effectName + "_yellowReduction"])
        params.yellowReduction = config[effectName + "_yellowReduction"].as<float>();
      if (config[effectName + "_alphaMultiplier"])
        params.alphaMultiplier = config[effectName + "_alphaMultiplier"].as<float>();
      if (config[effectName + "_textureInfluence"])
        params.textureInfluence = config[effectName + "_textureInfluence"].as<float>();
      if (config[effectName + "_baseColor"]) {
        auto baseColor = config[effectName + "_baseColor"].as<std::vector<float>>();
        for (int i = 0; i < 3 && i < baseColor.size(); i++) {
          params.baseColor[i] = baseColor[i];
        }
      }

      if (config[effectName + "_endColor"]) {
        auto endColor = config[effectName + "_endColor"].as<std::vector<float>>();
        for (int i = 0; i < 3 && i < endColor.size(); i++) {
          params.endColor[i] = endColor[i];
        }
      }
    };

    loadEffectParams("fire", preset.params.fire);
    loadEffectParams("smoke", preset.params.smoke);
    loadEffectParams("sparks", preset.params.sparks);
    loadEffectParams("ash", preset.params.ash);
    loadEffectParams("heat", preset.params.heat);

    return true;
  } catch (const std::exception &e) {
    std::cerr << "Error loading preset from " << filename << ": " << e.what() << std::endl;
    return false;
  }
}

void FireParamsPanel::ApplyParamsToShader() {
  auto &renderer = engine::Renderer<engine::graphics::OpenGLPolicy>::Instance();

  // Обновляем пути текстур из UI в параметры
  m_params.fireTexturePath = m_texturePaths[0];
  m_params.smokeTexturePath = m_texturePaths[1];
  m_params.sparksTexturePath = m_texturePaths[2];
  m_params.ashTexturePath = m_texturePaths[3];
  m_params.heatTexturePath = m_texturePaths[4];

  renderer.SetFireParams(m_params);
}

// Остальные методы (DrawPresetsSection, SavePreset, LoadPreset, DeletePreset,
// RefreshPresetsList, SavePresetToYAML, LoadPresetFromYAML)
// нужно адаптировать под новую структуру FireParams

}  // namespace ui