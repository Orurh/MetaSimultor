// EditorContext helper
#include "ui/Editor.h"
#include "ui/EditorContext.h"

namespace ui {
ToolType EditorContext::GetActiveToolType() const noexcept {
  return (m_editor != nullptr) ? m_editor->ActiveToolType()
                               : ToolType::OTHER_TOOL;
}
} // namespace ui
