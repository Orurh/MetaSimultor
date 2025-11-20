// ui/IPanel.h
#pragma once
#include "EditorContext.h"
#include "ui/Interface.h"

namespace ui {
enum class Workspace : uint8_t;

struct IPanel : public Interface {
  [[nodiscard]] virtual const char* Id() const = 0;  // стабильный док-ID
  virtual void Draw(EditorContext& ctx) = 0;

  virtual bool IsVisibleInWorkspace(EditorContext& ctx) const {
    (void)ctx;
    return true;
  }


  // NEW: жизненный цикл показа/скрытия окна
  virtual void OnShow(EditorContext& ctx) {}  // по умолчанию no-op
  virtual void OnHide(EditorContext& ctx) {}  // по умолчанию no-op
};
}  // namespace ui
