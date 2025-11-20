// ui/modules/SceneModule.cc
#include "SceneModule.h"
#include "ui/Editor.h"
#include "ui/panels/SceneIOPanel.h"
#include "ui/panels/TimePanel.h"
#include "ui/panels/ToolbarPanel.h"
#include "ui/panels/WindPanel.h"

namespace ui {

void SceneModule::Startup(Editor& editor) {
  editor.RegisterPanel(std::make_unique<ToolbarPanel>());
  editor.RegisterPanel(std::make_unique<SceneIOPanel>());
  editor.RegisterPanel(std::make_unique<WindPanel>());
  editor.RegisterPanel(std::make_unique<TimePanel>());
}

}  // namespace ui
