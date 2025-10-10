#include "Application.h"
#include <stdexcept>
#include <thread>
#include <iostream>
namespace engine {
Application::Application(int width, int height, const std::string &title) : m_window(width, height, title)
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }
    glViewport(0, 0, width, height);
  
    m_scene.Init();
}

void Application::Run()
{
    const double TARGET_FPS = 60.0;
    const double FRAME_TIME = 1.0 / TARGET_FPS;
    
    while(!m_window.ShouldClose()) {
        double frameStart = glfwGetTime();
        
        float currentFrame = static_cast<float>(glfwGetTime());
        float dt = currentFrame - m_lastFrameTime;
        m_lastFrameTime = currentFrame;
        m_window.PollEvents();  

        m_scene.Update(dt);
        m_scene.Render();
        m_window.SwapBuffers();
    }
}
}