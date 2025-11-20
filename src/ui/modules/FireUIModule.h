// ui/modules/FireUIModule.h
#pragma once
#include "ui/IModule.h"

namespace ui {

class FireUIModule final : public IModule {
public:
  void Startup(Editor& editor) override;
  void Shutdown() override {}
};

} // namespace ui
