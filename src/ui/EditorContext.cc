#include "ui/EditorContext.h"
#include "ui/Editor.h"   // здесь уже можно, цикл не страшен в .cc

namespace ui {

EditorState& EditorContext::State() {
  return m_editor->m_state;
}

const EditorState& EditorContext::State() const {
  return m_editor->m_state;
}


} // namespace ui
