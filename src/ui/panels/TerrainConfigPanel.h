// src/ui/panels/TerrainConfigPanel.h
#pragma once
#include "core/scene/Terrain.h"
#include "ui/IPanel.h"
#include <glm/glm.hpp>

namespace ui {

class TerrainConfigPanel final : public IPanel {
public:
  [[nodiscard]] const char *Id() const override { return "Terrain Config"; }

  void Draw(EditorContext &ctx) override;
  bool IsVisibleInWorkspace(EditorContext& ctx) const override {
    using W = EditorState::Workspace;
    const auto WS = ctx.State().m_workspace;
    return WS == W::TERRAIN || WS == W::ALL;
  }
private:
  void DrawTextureSettings(EditorContext &ctx);
  void DrawTexturePreview();
  void LoadAndPreviewTexture(const std::string &path);
  engine::TerrainGenerationSettings m_settings;
  std::shared_ptr<engine::Texture> m_texturePreview;
  bool m_regenerate = false;
  bool m_textureChanged = false;
  bool m_textureLoaded = false;
  std::string m_texturePreviewPath;
  char m_texturePath[256];
};

} // namespace ui