#pragma once

#include "configs/FireParams.h"
#include "ui/EditorContext.h"
#include "ui/IPanel.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include "graphics/Texture.h"


namespace ui {

class FireParamsPanel final : public IPanel {
public:
  FireParamsPanel() { 
    RefreshPresetsList();
    LoadAndPreviewTexture(texturePath);
  }
  void Draw(EditorContext &ctx) override;
  const char* Id() const override { return "Fire Parameters"; }

private:
  FireParams m_params;
  
  // Система пресетов
  struct Preset {
    std::string name;
    FireParams params;
    std::string texturePath;
  };
  
  std::unordered_map<std::string, Preset> m_presets;
  std::string m_currentPresetName;
  char m_newPresetName[64] = "";
  std::string m_presetsDirectory = "presets";
  char texturePath[256] = "res/textures/fire_texture3.png";
  std::chrono::steady_clock::time_point m_lastUpdateTime;
  static constexpr std::chrono::milliseconds UPDATE_DELAY{16}; // ~60 FPS
  bool m_needsUpdate = false;

  std::shared_ptr<engine::Texture> m_currentTexture;
  bool m_textureLoaded = false;
  std::string m_lastTexturePath;
  void DrawPresetsSection();
  void SavePreset(const std::string& name);
  void LoadPreset(const std::string& name);
  void DeletePreset(const std::string& name);
  void RefreshPresetsList();
  void SavePresetToYAML(const Preset& preset);
  bool LoadPresetFromYAML(const std::string& filename, Preset& preset);
  void LoadAndPreviewTexture(const std::string& path);
  void DrawTexturePreview();
};

} // namespace ui