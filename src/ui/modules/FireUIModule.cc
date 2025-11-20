// ui/modules/FireUIModule.cc
#include "FireUIModule.h"
#include "ui/Editor.h"
#include "ui/panels/FireDebugPanel.h"
#include "ui/panels/FireParamsPanel.h"
#include "ui/panels/ParticleSystemPanel.h"

namespace ui {

void FireUIModule::Startup(Editor& editor) {
  editor.RegisterPanel(std::make_unique<FireDebugPanel>());
  editor.RegisterPanel(std::make_unique<FireParamsPanel>());
  editor.RegisterPanel(std::make_unique<ParticleSystemPanel>());
}

} // namespace ui
