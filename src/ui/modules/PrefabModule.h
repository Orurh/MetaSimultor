// ui/modules/PrefabModule.h
#pragma once
#include "ui/IModule.h"

namespace ui {

class PrefabModule final : public IModule {
public:
  void Startup(Editor& editor) override;
  void Shutdown() override {}
};

} // namespace ui
