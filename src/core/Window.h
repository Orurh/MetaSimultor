#pragma once 
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include "Input.h"
namespace engine {
class Window {
static void mouse_position_callback(GLFWwindow* window, double xpos, double ypos) {
    auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win) {
        InputSystem::Instance().SetMousePosition(xpos, ypos);
    }
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win) {
        InputSystem::Instance().SetMouseButtonState(button, action == GLFW_PRESS);
    }
}
static void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win) {
        InputSystem::Instance().SetMouseScrollY(yoffset);
    }
}
    public:
        Window(int width, int height, const std::string& title);
        ~Window();
        void PollEvents();
        bool ShouldClose() const;
        void SwapBuffers();
        GLFWwindow* GetWindow() const;
    private:
        GLFWwindow* m_window;
};
}