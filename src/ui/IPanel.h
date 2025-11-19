// ui/IPanel.h
#pragma once
#include "ui/EditorContext.h"
#include "ui/Interface.h"

namespace ui {
struct IPanel : public Interface {
  [[nodiscard]] virtual const char* Id() const = 0;  // стабильный док-ID
  virtual void Draw(EditorContext& ctx) = 0;

  // NEW: жизненный цикл показа/скрытия окна
  virtual void OnShow(EditorContext& ctx) {}  // по умолчанию no-op
  virtual void OnHide(EditorContext& ctx) {}  // по умолчанию no-op
};
}  // namespace ui
