// ui/panels/TimePanel.h
#pragma once

#include "ui/IPanel.h"  // вместо EditorContext

namespace ui {

class TimePanel final : public IPanel {
public:
  TimePanel() = default;

  [[nodiscard]] const char* Id() const override { return "Time"; }
  // Name можно либо вообще убрать, либо оставить без override:
  [[nodiscard]] const char* Name() const { return "Time"; }

  void Draw(EditorContext &ctx) override;

  bool& Visible() { return m_visible; }

private:
  bool m_visible = true;
};

} // namespace ui
