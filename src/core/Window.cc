#include "Window.h"
#include <stdexcept>
namespace engine {
Window::Window(int width, int height, const std::string &title)
{
    if(!glfwInit()) {
        throw std::runtime_error("Failed to start GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if(!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to initialize window");
    }
    glfwMakeContextCurrent(m_window);
}
Window::~Window()
{
    if(m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}
void Window::PollEvents()
{
}
bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(m_window);
}
void Window::SwapBuffers()
{
   glfwSwapBuffers(m_window); 
}
GLFWwindow *Window::GetWindow() const
{
    return nullptr;
}
}
