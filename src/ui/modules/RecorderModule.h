#pragma once
#include "ui/IModule.h"

namespace ui {

class RecorderModule : public IModule {
  public:
  void Startup(Editor &editor) override;
  void Shutdown() override {}
};

}  // namespace ui