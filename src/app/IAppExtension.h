// src/app/IAppExtension.h
#pragma once
#include <memory>

namespace engine { class Application; }
namespace ui { struct EditorContext; }

struct IAppExtension {

  // вызываться один раз, когда окно и GL уже готовы
  virtual void OnInit(engine::Application& app) = 0;

  virtual void OnBeginUI(engine::Application& app, ui::EditorContext& ctx) = 0;
  virtual void OnEndUI(engine::Application& app, ui::EditorContext& ctx) = 0;

  virtual void OnShutdown(engine::Application& app) = 0;
};
