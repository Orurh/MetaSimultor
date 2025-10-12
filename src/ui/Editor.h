// ui/Editor.h
#pragma once
#include <memory>
#include <ranges>
#include <type_traits>
#include <vector>

#include "ui/IModule.h"
#include "ui/IPanel.h"
#include "ui/ITool.h"
// не инклюдим IModule тут — достаточно fwd-decl
namespace ui {
struct IModule;
} // namespace ui

namespace ui {

class Editor {
public:
  Editor() = default;

  // Запрет копирования
  Editor(const Editor &) = delete;
  Editor &operator=(const Editor &) = delete;

  // Перемещение: переносим владение, но сбрасываем стек (переактивация позже)
  Editor(Editor &&other) noexcept
      : m_panels(std::move(other.m_panels)), m_tools(std::move(other.m_tools)),
        m_modules(std::move(other.m_modules)) {
    m_activeStack.clear();
  }

  Editor &operator=(Editor &&other) noexcept {
    if (this != &other) {
      ShutdownModules(); // на всякий случай корректно выгружаем свои
      m_panels = std::move(other.m_panels);
      m_tools = std::move(other.m_tools);
      m_modules = std::move(other.m_modules);
      m_activeStack.clear();
    }
    return *this;
  }

  ~Editor() {
    // Гарантируем Shutdown модулей перед разрушением редактора
    for (auto &m_module : std::ranges::reverse_view(m_modules)) {
      m_module->Shutdown();
    }
  }

  // --- Регистрация панелей/инструментов (как было) ---
  void RegisterPanel(std::unique_ptr<IPanel> panel) {
    m_panels.emplace_back(std::move(panel));
  }
  void RegisterTool(std::unique_ptr<ITool> tool) {
    m_tools.emplace_back(std::move(tool));
  }

  template <class T> T *GetTool() {
    static_assert(std::is_base_of_v<ITool, T>, "T must be ITool");
    for (auto &up : m_tools) {
      if (auto *tool_pointer = dynamic_cast<T *>(up.get())) {
        return tool_pointer;
      }
    }
    return nullptr;
  }
  template <class T> const T *GetTool() const {
    static_assert(std::is_base_of_v<ITool, T>, "T must be ITool");
    for (const auto &up : m_tools) {
      if (auto *tool_pointer = dynamic_cast<const T *>(up.get())) {
        return tool_pointer;
      }
    }
    return nullptr;
  }

  // --- Модули ---
  void RegisterModule(std::unique_ptr<IModule> module) {
    m_modules.emplace_back(std::move(module));
    m_modules.back()->Startup(*this);
  }

  // При желании — явная остановка модулей до деструктора
  void ShutdownModules() {
    for (auto &m_module : std::ranges::reverse_view(m_modules)) {
      m_module->Shutdown();
    }
  }

  // --- Активные инструменты (как уже сделали) ---
  void SetActiveTool(ToolType type, EditorContext &ctx) {
    if (!m_activeStack.empty()) {
      m_activeStack.back()->OnDeactivate(ctx);
      m_activeStack.clear();
    }
    if (auto *tool = FindTool(type)) {
      m_activeStack.push_back(tool);
      tool->OnActivate(ctx);
    }
  }
  void PushTool(ToolType type, EditorContext &ctx) {
    if (auto *tool = FindTool(type)) {
      if (!m_activeStack.empty()) {
        m_activeStack.back()->OnDeactivate(ctx);
      }
      m_activeStack.push_back(tool);
      tool->OnActivate(ctx);
    }
  }
  void PopTool(EditorContext &ctx) {
    if (m_activeStack.empty()) {
      return;
    }
    m_activeStack.back()->OnDeactivate(ctx);
    m_activeStack.pop_back();
    if (!m_activeStack.empty()) {
      m_activeStack.back()->OnActivate(ctx);
    }
  }

  void DrawUI(EditorContext &ctx) {
    ctx.m_editor = this;
    for (auto &panel : m_panels) {
      panel->Draw(ctx);
    }
  }
  void TickTools(EditorContext &ctx) {
    if (!m_activeStack.empty()) {
      m_activeStack.back()->OnUpdate(ctx);
    }
  }

  [[nodiscard]] ToolType ActiveToolType() const {
    return m_activeStack.empty() ? ToolType::OTHER_TOOL
                                 : m_activeStack.back()->Type();
  }

private:
  ITool *FindTool(ToolType type) {
    for (auto &tool : m_tools) {
      if (tool->Type() == type) {
        return tool.get();
      }
    }
    return nullptr;
  }

  std::vector<std::unique_ptr<IPanel>> m_panels;
  std::vector<std::unique_ptr<ITool>> m_tools;
  std::vector<ITool *> m_activeStack;
  std::vector<std::unique_ptr<IModule>> m_modules; // <-- NEW
};

} // namespace ui
