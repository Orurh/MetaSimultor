// ui/tools/PlaceOnGroundTool.h
#pragma once
#include "ui/ITool.h"

namespace ui {
class PlaceOnGroundTool final : public ITool {
public:
  explicit PlaceOnGroundTool(const char* prefabPath);

  [[nodiscard]] ToolType Type() const noexcept override;
  void OnActivate(EditorContext& ctx) override;   // NEW
  void OnDeactivate(EditorContext& ctx) override; // NEW
  void OnUpdate(EditorContext& ctx) override;

private:
  const char* m_prefabPath;
  bool        m_isActive = false; // флаг для локального состояния
};
} // namespace ui
