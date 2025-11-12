// ui/panels/ToolbarPanel.h
#pragma once
#include "ui/IPanel.h"
#include <imgui.h>
#include "FoliageBrushTool.h"

namespace ui {
class ToolbarPanel final : public IPanel {
public:
  [[nodiscard]] const char* Id() const override { return "Toolbar"; }

  void Draw(EditorContext& ctx) override;
  void OnShow(EditorContext& /*ctx*/) override { m_wasVisible = true; }
  void OnHide(EditorContext& /*ctx*/) override { m_wasVisible = false; }

private:
  void DrawModeSwitch(EditorContext& ctx);     // Prefab / Fire
  void DrawToolSelector(EditorContext& ctx);   // Place on ground / Foliage Brush
  void DrawActiveToolUI(EditorContext& ctx);   // UI активного инструмента (в т.ч. кисти)

  bool  m_wasVisible = false;
  float m_fireIntensity = 1.0F;
  float m_fireX = 510.0f;   // мировая X
  float m_fireY = 501.0f;   // мировая Z (у тебя второй параметр — это Z)
};
} // namespace ui
