#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include "configs/FireParams.h"
#include "graphics/Texture.h"
#include "ui/EditorContext.h"
#include "ui/IPanel.h"

namespace ui {

class FireParamsPanel final : public IPanel {
 public:
  FireParamsPanel() {
    RefreshPresetsList();
    // Загружаем preview для всех текстур
    LoadAndPreviewTexture(m_params.fireTexturePath, 0);
    LoadAndPreviewTexture(m_params.smokeTexturePath, 1);
    LoadAndPreviewTexture(m_params.sparksTexturePath, 2);
    LoadAndPreviewTexture(m_params.ashTexturePath, 3);
    LoadAndPreviewTexture(m_params.heatTexturePath, 4);
  }
  bool IsVisibleInWorkspace(EditorContext& ctx) const override {
    using W = EditorState::Workspace;
    const auto WS = ctx.State().m_workspace;
    return WS == W::FIRE || WS == W::FX || WS == W::ALL;
  }
  void Draw(EditorContext& ctx) override;
  const char* Id() const override { return "Fire & Effects"; }

 private:
  FireParams m_params;

  // Система пресетов
  struct Preset {
    std::string name;
    FireParams params;
  };

  std::unordered_map<std::string, Preset> m_presets;
  std::string m_currentPresetName;
  char m_newPresetName[64] = "";
  std::string m_presetsDirectory = "presets";

  // Таймер для дебаунсинга
  std::chrono::steady_clock::time_point m_lastUpdateTime;
  static constexpr std::chrono::milliseconds UPDATE_DELAY{16};
  bool m_needsUpdate = false;

  // Preview текстур для всех эффектов
  std::shared_ptr<engine::Texture> m_texturePreviews[5];  // 0=fire, 1=smoke, 2=sparks, 3=ash, 4=heat
  bool m_textureLoaded[5] = {false};
  char m_texturePaths[5][256] = {"res/textures/fire_texture3.png", "res/textures/smoke_texture.png", "res/textures/sparks_texture.png",
                                 "res/textures/ash_texture.png", "res/textures/heat_texture.png"};

  // Текущая активная вкладка
  int m_currentTab = 0;

  void DrawPresetsSection();
  void DrawGlobalTab();
  void DrawFireTab();
  void DrawSmokeTab();
  void DrawSparksTab();
  void DrawAshTab();
  void DrawHeatTab();
  void DrawTexturesTab();
  void DrawEffectParams(const char* name, FireParams::EffectParams& params, int effectType, bool showColor = true);

  void SavePreset(const std::string& name);
  void LoadPreset(const std::string& name);
  void DeletePreset(const std::string& name);
  void RefreshPresetsList();
  void SavePresetToYAML(const Preset& preset);
  bool LoadPresetFromYAML(const std::string& filename, Preset& preset);

  void LoadAndPreviewTexture(const std::string& path, int textureIndex);
  void DrawTexturePreview(int textureIndex, const char* name);
  void ApplyParamsToShader();
};

}  // namespace ui