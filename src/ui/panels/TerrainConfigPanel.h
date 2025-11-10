// src/ui/panels/TerrainConfigPanel.h
#pragma once
#include "ui/IPanel.h"
namespace ui {

class TerrainConfigPanel final : public IPanel {
public:
  [[nodiscard]] const char *Id() const override { return "Terrain Config"; }

  void Draw(EditorContext &ctx) override;

private:
  struct Settings {
    int width = 256;
    int depth = 256;
    float scale = 2.0f;
    float maxHeight = 80.0f;
    uint32_t seed = 42;
    float frequency = 0.02f;
    int octaves = 6;
    float persistence = 0.5f;
    float lacunarity = 2.0f;
  } m_settings;

  bool m_regenerate = false;
};

} // namespace ui