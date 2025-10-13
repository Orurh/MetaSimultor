// modules/BasicToolsModule.h
#pragma once
#include "ui/IModule.h"

namespace ui {
class BasicToolsModule final : public IModule {
public:
  void Startup(Editor& editor) override;
  void Shutdown() override;
};
} // namespace ui
