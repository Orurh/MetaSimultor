// ui/modules/SceneModule.h
#pragma once
#include "ui/IModule.h"

namespace ui {

class SceneModule final : public IModule {
public:
  void Startup(Editor& editor) override;
  void Shutdown() override {}  // ничего своего не держим
};

} // namespace ui
