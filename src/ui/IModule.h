// ui/IModule.h
#pragma once
#include "ui/Interface.h"

namespace ui {
class Editor; // fwd-decl

struct IModule : public Interface {

  // Регистрация панелей/инструментов и другой интеграции
  virtual void Startup(Editor& editor) = 0;

  // Освобождение ресурсов (если нужно)
  virtual void Shutdown() = 0;
};
} // namespace ui
