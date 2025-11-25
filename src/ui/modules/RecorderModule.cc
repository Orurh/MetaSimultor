// ui/modules/PrefabModule.cc
#include "RecorderModule.h"
#include "ui/Editor.h"
#include "ui/panels/RecordingPanel.h"

namespace ui {

void RecorderModule::Startup(Editor &editor) { editor.RegisterPanel(std::make_unique<RecordingPanel>()); }

}  // namespace ui
