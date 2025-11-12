#include "FireParamsPanel.h"
#include "graphics/Renderer.h"
#include "ui/EditorContext.h"
#include <imgui.h>
#include <iostream>
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace ui {

void FireParamsPanel::Draw(EditorContext &ctx) {
  if (!ImGui::Begin(Id())) {
    ImGui::End();
    return;
  }

  DrawPresetsSection();
  
  ImGui::Separator();

  if (ImGui::Button("Reset to Default")) {
    m_params = FireParams();
  }
  ImGui::SameLine();
  if (ImGui::Button("Apply to Shader")) {
    auto &renderer = engine::Renderer::Instance();
    renderer.SetFireParams(m_params);
  }
  bool changed = false;
  ImGui::SeparatorText("Main Parameters");

  changed |= ImGui::SliderFloat("Base Intensity", &m_params.baseIntensity, 0.1f, 5.0f);
  changed |=ImGui::SliderFloat("Particle Size", &m_params.particleSize, 0.5f, 10.0f);
  changed |=ImGui::SliderFloat("Size Variation", &m_params.sizeVariation, 0.0f, 2.0f);
  ImGui::SeparatorText("Animation");
  changed |=ImGui::SliderFloat("Animation Speed", &m_params.animationSpeed, 0.1f, 5.0f);
  changed |=ImGui::SliderFloat("Particle Lifetime", &m_params.particleLifetime, 0.5f,
                      10.0f);
  changed |=ImGui::SliderFloat("Spawn Rate", &m_params.spawnRate, 10.0f, 1000.0f);
  ImGui::SeparatorText("Movement");
  changed |=ImGui::SliderFloat("Rise Speed", &m_params.riseSpeed, 0.1f, 10.0f);
  changed |=ImGui::SliderFloat("Turbulence", &m_params.turbulence, 0.0f, 2.0f);
  changed |=ImGui::SliderFloat("Wind Influence", &m_params.windInfluence, 0.0f, 5.0f);
  ImGui::SeparatorText("Visual Effects");
  changed |=ImGui::SliderFloat("Red Intensity", &m_params.redIntensity, 0.0f, 2.0f);
  changed |=ImGui::SliderFloat("Yellow Reduction", &m_params.yellowReduction, 0.0f, 1.0f);
  changed |=ImGui::SliderFloat("Alpha Multiplier", &m_params.alphaMultiplier, 0.1f, 3.0f);
  ImGui::SeparatorText("Particle Distribution");
  changed |=ImGui::SliderFloat("Horizontal Spread", &m_params.horizontalSpread, 0.1f,
                     2.0f);
  changed |=ImGui::SliderFloat("Vertical Spread", &m_params.verticalSpread, 0.1f, 2.0f);

  ImGui::SeparatorText("Colors");
  changed |=ImGui::ColorEdit3("Base Color", &m_params.baseColor[0]);
  changed |=ImGui::ColorEdit3("End Color", &m_params.endColor[0]);
  ImGui::SeparatorText("Fire Texture");

  if (ImGui::InputText("Texture Path", texturePath, sizeof(texturePath))) {
      m_params.fireTexturePath = texturePath;
      LoadAndPreviewTexture(texturePath);
      changed = true;
  }

  if (ImGui::Button("Load Texture")) {
      m_params.fireTexturePath = texturePath;
      LoadAndPreviewTexture(texturePath);
      changed = true;
      std::cout << "Texture loaded: " << texturePath << std::endl;
  }
  
  ImGui::SameLine();
  
  if (ImGui::Button("Preview")) {
      LoadAndPreviewTexture(texturePath);
  }

  // Preview текстуры
  DrawTexturePreview();

  changed |= ImGui::SliderFloat("Texture Influence", &m_params.textureInfluence, 0.0f, 1.0f, "%.2f");
  ImGui::TextDisabled("0.0 = only params color, 1.0 = only texture color");
  
  if (changed) {
    m_needsUpdate = true;
    m_lastUpdateTime = std::chrono::steady_clock::now();
  }
    
    // Применяем изменения с задержкой
  auto now = std::chrono::steady_clock::now();
  if (m_needsUpdate && (now - m_lastUpdateTime) > UPDATE_DELAY) {
    auto &renderer = engine::Renderer::Instance();
    renderer.SetFireParams(m_params);
    m_needsUpdate = false;
  }
  ImGui::End();
}
void FireParamsPanel::LoadAndPreviewTexture(const std::string& path) {
    try {
        if (std::filesystem::exists(path)) {
            m_currentTexture = engine::Texture::Create(path);
            m_textureLoaded = true;
            m_lastTexturePath = path;
            std::cout << "Texture preview loaded: " << path << std::endl;
        } else {
            m_textureLoaded = false;
            std::cout << "Texture file not found: " << path << std::endl;
        }
    }
    catch (const std::exception& e) {
        m_textureLoaded = false;
        std::cerr << "Failed to load texture: " << e.what() << std::endl;
    }

}

void FireParamsPanel::DrawTexturePreview() {
    if (!m_textureLoaded || !m_currentTexture) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Texture not loaded");
        
        // Placeholder
        ImGui::BeginChild("TexturePlaceholder", ImVec2(200, 200), true, ImGuiWindowFlags_NoScrollbar);
        ImVec2 size = ImGui::GetContentRegionAvail();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImVec2 p1 = ImVec2(p0.x + size.x, p0.y + size.y);
        
        // Рисуем серый квадрат
        draw_list->AddRectFilled(p0, p1, IM_COL32(50, 50, 50, 255));
        draw_list->AddRect(p0, p1, IM_COL32(100, 100, 100, 255));
        
        // Текст по центру
        ImVec2 text_size = ImGui::CalcTextSize("No Texture");
        ImVec2 text_pos = ImVec2(p0.x + (size.x - text_size.x) * 0.5f, 
                                p0.y + (size.y - text_size.y) * 0.5f);
        draw_list->AddText(text_pos, IM_COL32(150, 150, 150, 255), "No Texture");
        
        ImGui::EndChild();
        return;
    }
    
    ImGui::Text("Texture Preview:");
    
    auto texture_id = m_currentTexture->GetId();
    
    if (texture_id) {
        float preview_size = 200.0f;
        
        ImGui::BeginChild("TexturePreview", ImVec2(preview_size + 10, preview_size + 50), true);
        {
            // ИСПРАВЛЕННЫЙ ВЫЗОВ - для новой версии ImGui
            ImGui::Image(texture_id, ImVec2(preview_size, preview_size));
            
            // Информация под preview
            ImGui::Text("Size: %dx%d", m_currentTexture->GetWidth(), m_currentTexture->GetHeight());
            ImGui::Text("File: %s", std::filesystem::path(texturePath).filename().string().c_str());
        }
        ImGui::EndChild();
    } else {
        // Если не получили ID - показываем информацию о текстуре
        ImGui::BeginChild("TextureInfo", ImVec2(200, 100), true);
        {
            ImGui::Text("Texture Info:");
            ImGui::Text("Size: %dx%d", m_currentTexture->GetWidth(), m_currentTexture->GetHeight());
            ImGui::Text("File: %s", std::filesystem::path(texturePath).filename().string().c_str());
            ImGui::Text("Status: Loaded");
        }
        ImGui::EndChild();
    }
    
    // Кнопки управления
    ImGui::BeginGroup();
    if (ImGui::Button("Reload Texture")) {
        LoadAndPreviewTexture(texturePath);
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Preview")) {
        m_textureLoaded = false;
        m_currentTexture.reset();
    }
    ImGui::EndGroup();
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
    
    for (const auto& [name, preset] : m_presets) {
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

void FireParamsPanel::SavePreset(const std::string& name) {
  Preset preset;
  preset.name = name;
  preset.params = m_params;
  preset.texturePath = m_params.fireTexturePath;
  
  SavePresetToYAML(preset);
  m_currentPresetName = name;
  
  std::cout << "Preset saved: " << name << std::endl;
}

void FireParamsPanel::LoadPreset(const std::string& name) {
  std::string filename = m_presetsDirectory + "/" + name + ".yaml";
  Preset preset;
  
  if (LoadPresetFromYAML(filename, preset)) {
    m_params = preset.params;
    m_currentPresetName = name;
    
    strncpy(texturePath, preset.texturePath.c_str(), sizeof(texturePath)-1);
    texturePath[sizeof(texturePath)-1] = '\0';

    m_params.fireTexturePath = texturePath;
    auto &renderer = engine::Renderer::Instance();
    renderer.SetFireParams(m_params);
  }
}

void FireParamsPanel::DeletePreset(const std::string& name) {
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
  
  for (const auto& entry : std::filesystem::directory_iterator(m_presetsDirectory)) {
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

void FireParamsPanel::SavePresetToYAML(const Preset& preset) {
  YAML::Emitter out;
  
  out << YAML::BeginMap;
  out << YAML::Key << "name" << YAML::Value << preset.name;
  out << YAML::Key << "texturePath" << YAML::Value << preset.texturePath;
  
  // Main Parameters
  out << YAML::Key << "baseIntensity" << YAML::Value << preset.params.baseIntensity;
  out << YAML::Key << "particleSize" << YAML::Value << preset.params.particleSize;
  out << YAML::Key << "sizeVariation" << YAML::Value << preset.params.sizeVariation;
  
  // Animation
  out << YAML::Key << "animationSpeed" << YAML::Value << preset.params.animationSpeed;
  out << YAML::Key << "particleLifetime" << YAML::Value << preset.params.particleLifetime;
  out << YAML::Key << "spawnRate" << YAML::Value << preset.params.spawnRate;
  
  // Movement
  out << YAML::Key << "riseSpeed" << YAML::Value << preset.params.riseSpeed;
  out << YAML::Key << "turbulence" << YAML::Value << preset.params.turbulence;
  out << YAML::Key << "windInfluence" << YAML::Value << preset.params.windInfluence;
  
  // Visual Effects
  out << YAML::Key << "redIntensity" << YAML::Value << preset.params.redIntensity;
  out << YAML::Key << "yellowReduction" << YAML::Value << preset.params.yellowReduction;
  out << YAML::Key << "alphaMultiplier" << YAML::Value << preset.params.alphaMultiplier;
  
  // Particle Distribution
  out << YAML::Key << "horizontalSpread" << YAML::Value << preset.params.horizontalSpread;
  out << YAML::Key << "verticalSpread" << YAML::Value << preset.params.verticalSpread;
  
  // Colors
  out << YAML::Key << "baseColor" << YAML::Value << YAML::Flow << YAML::BeginSeq
      << preset.params.baseColor[0] << preset.params.baseColor[1] << preset.params.baseColor[2]
      << YAML::EndSeq;
  out << YAML::Key << "endColor" << YAML::Value << YAML::Flow << YAML::BeginSeq
      << preset.params.endColor[0] << preset.params.endColor[1] << preset.params.endColor[2]
      << YAML::EndSeq;
  
  // Texture
  out << YAML::Key << "textureInfluence" << YAML::Value << preset.params.textureInfluence;
  
  out << YAML::EndMap;
  
  std::string filename = m_presetsDirectory + "/" + preset.name + ".yaml";
  std::ofstream file(filename);
  file << out.c_str();
}

bool FireParamsPanel::LoadPresetFromYAML(const std::string& filename, Preset& preset) {
  try {
    YAML::Node config = YAML::LoadFile(filename);
    
    preset.name = config["name"].as<std::string>();
    preset.texturePath = config["texturePath"].as<std::string>();
    
    // Main Parameters
    preset.params.baseIntensity = config["baseIntensity"].as<float>();
    preset.params.particleSize = config["particleSize"].as<float>();
    preset.params.sizeVariation = config["sizeVariation"].as<float>();
    
    // Animation
    preset.params.animationSpeed = config["animationSpeed"].as<float>();
    preset.params.particleLifetime = config["particleLifetime"].as<float>();
    preset.params.spawnRate = config["spawnRate"].as<float>();
    
    // Movement
    preset.params.riseSpeed = config["riseSpeed"].as<float>();
    preset.params.turbulence = config["turbulence"].as<float>();
    preset.params.windInfluence = config["windInfluence"].as<float>();
    
    // Visual Effects
    preset.params.redIntensity = config["redIntensity"].as<float>();
    preset.params.yellowReduction = config["yellowReduction"].as<float>();
    preset.params.alphaMultiplier = config["alphaMultiplier"].as<float>();
    
    // Particle Distribution
    preset.params.horizontalSpread = config["horizontalSpread"].as<float>();
    preset.params.verticalSpread = config["verticalSpread"].as<float>();
    
    // Colors
    auto baseColor = config["baseColor"].as<std::vector<float>>();
    auto endColor = config["endColor"].as<std::vector<float>>();
    for (int i = 0; i < 3; i++) {
      preset.params.baseColor[i] = baseColor[i];
      preset.params.endColor[i] = endColor[i];
    }
    
    // Texture
    preset.params.textureInfluence = config["textureInfluence"].as<float>();
    preset.params.fireTexturePath = preset.texturePath;
    
    return true;
  }
  catch (const std::exception& e) {
    std::cerr << "Error loading preset from " << filename << ": " << e.what() << std::endl;
    return false;
  }
}

} // namespace ui