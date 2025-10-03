#pragma once 
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
namespace engine {
class Window {
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