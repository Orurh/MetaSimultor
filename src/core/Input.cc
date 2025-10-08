#include "Input.h"
#include <iostream>
void engine::InputSystem::Update() {
    // Сохраняем предыдущие состояния
    m_previousKeyState = m_currentKeyState;
    m_prevMouseButtonState = m_mouseButtonState;
    
    // Сбрасываем дельту мыши (она обновляется каждый кадр)
}
bool engine::InputSystem::GetKey(KeyCode key, InputState state) const {
  auto currentIt = m_currentKeyState.find(key);
  auto previousIt = m_previousKeyState.find(key);

  bool current =
      currentIt != m_currentKeyState.end() ? currentIt->second : false;
  bool previous =
      previousIt != m_previousKeyState.end() ? previousIt->second : false;

  switch (state) {
  case InputState::Held:
    return current;
  case InputState::Pressed:
    return current && !previous;
  case InputState::Released:
    return !current && previous;
  }
  return false;
}
void engine::InputSystem::SetMousePosition(float x, float y) {
    if (m_mousePosition != glm::vec2(0.0f)) {
        m_mouseDelta = glm::vec2(x, y) - m_mousePosition;
    }
    m_mousePosition = glm::vec2(x, y);
}

void engine::InputSystem::SetMouseButtonState(int button, bool pressed) {
    m_mouseButtonState[button] = pressed;
}


bool engine::InputSystem::GetMouseButton(int button, InputState state) const {
    auto currentIt = m_mouseButtonState.find(button);
    auto previousIt = m_prevMouseButtonState.find(button);

    bool current = currentIt != m_mouseButtonState.end() ? currentIt->second : false;
    bool previous = previousIt != m_prevMouseButtonState.end() ? previousIt->second : false;

    switch (state) {
    case InputState::Held:
        return current;
    case InputState::Pressed:
        return current && !previous;
    case InputState::Released:
        return !current && previous;
    }
    return false;
}
void engine::InputSystem::SetKeyState(KeyCode key, bool pressed)
{
    m_currentKeyState[key] = pressed;
}