// ui/ITool.h
#pragma once
#include "ui/EditorContext.h"
#include "ui/Interface.h"

namespace ui {

struct ITool : public Interface {

  // стабильный тип инструмента (вместо строк)
  [[nodiscard]] virtual ToolType Type() const noexcept = 0;
  [[nodiscard]] virtual const char* Name() const noexcept = 0;
  [[nodiscard]] virtual const char* Id() const noexcept = 0;

  // жизненный цикл
  virtual void OnActivate(EditorContext &ctx) = 0;   // включение/захват фокуса
  virtual void OnDeactivate(EditorContext &ctx) = 0; // выключение/очистка

  // поведение на тик
  virtual void OnUpdate(EditorContext &ctx) = 0;
};

} // namespace ui
