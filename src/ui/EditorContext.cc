#include "EditorContext.h"
#include "Editor.h"   // здесь уже можно, цикл не страшен в .cc

namespace ui {

EditorState& EditorContext::State() {
  return m_editor->GetState();
}

const EditorState& EditorContext::State() const {
  return m_editor->GetState();
}


} // namespace ui
