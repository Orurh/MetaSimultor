#include "Input.h"
#include <iostream>
void engine::InputSystem::Update() {
  m_previousKeyState       = m_currentKeyState;
  m_prevMouseButtonState   = m_mouseButtonState;

  m_mouseDelta   = glm::vec2(0.0f, 0.0f);
}

bool engine::InputSystem::GetKey(KeyCode key, InputState state) const {
  const bool currentIt  = (m_currentKeyState.count(key) != 0U)  ? m_currentKeyState.at(key)  : false;
  const bool previous = (m_previousKeyState.count(key) != 0U) ? m_previousKeyState.at(key) : false;

  switch (state) {
    case InputState::Held:     return currentIt;
    case InputState::Pressed:  return  currentIt && !previous;
    case InputState::Released: return !currentIt &&  previous;
  }
  return false;
}
void engine::InputSystem::SetMousePosition(float x, float y) {
  glm::vec2 newPos(x, y);
  m_mouseDelta     += (newPos - m_mousePosition); // накапливаем за кадр
  m_mousePosition   = newPos;
}

void engine::InputSystem::SetMouseButtonState(MouseButton button, bool pressed) {
  m_mouseButtonState[button] = pressed;
}


bool engine::InputSystem::GetMouseButton(MouseButton button, InputState state) const {
  const bool currentIt  = m_mouseButtonState.count(button)   ? m_mouseButtonState.at(button)   : false;
  const bool previousIt = m_prevMouseButtonState.count(button)? m_prevMouseButtonState.at(button): false;

  switch (state) {
    case InputState::Held:     return currentIt;
    case InputState::Pressed:  return  currentIt && !previousIt;
    case InputState::Released: return !currentIt &&  previousIt;
  }
  return false;
}
void engine::InputSystem::SetKeyState(KeyCode key, bool pressed)
{
    m_currentKeyState[key] = pressed;
}