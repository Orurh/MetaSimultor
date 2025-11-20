// ui/modules/TerrainModule.cc
#include "TerrainModule.h"
#include "ui/Editor.h"
#include "ui/panels/TerrainConfigPanel.h"

namespace ui {

void TerrainModule::Startup(Editor& editor) {
  editor.RegisterPanel(std::make_unique<TerrainConfigPanel>());
}

} // namespace ui
