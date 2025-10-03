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
    glm::mat4 view = glm::lookAt(
        glm::vec3(500.0f, 100.0f, 100.0f),  
        glm::vec3(500.0f, 0.0f, 500.0f),    
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f), 1200.0f/800.0f, 1.0f, 2000.0f
    );
    m_scene.Init();
    m_scene.SetViewProjection(view, projection);
}void Application::Run()
{
    const double TARGET_FPS = 60.0;
    const double FRAME_TIME = 1.0 / TARGET_FPS;
    
    while(!m_window.ShouldClose()) {
        double frameStart = glfwGetTime();
        
        float currentFrame = static_cast<float>(glfwGetTime());
        float dt = currentFrame - m_lastFrameTime;
        m_lastFrameTime = currentFrame;

        m_scene.Update(dt * 0.1);
        m_scene.Render();
        m_window.SwapBuffers();
        glfwPollEvents();
        
        double frameEnd = glfwGetTime();
        double frameDuration = frameEnd - frameStart;
        
        if (frameDuration < FRAME_TIME) {
            double sleepTime = FRAME_TIME - frameDuration;
            std::this_thread::sleep_for(
                std::chrono::duration<double>(sleepTime)
            );
        }
         static int frameCount = 0;
        static double lastFpsTime = glfwGetTime();
        frameCount++;
        
        if (frameEnd - lastFpsTime >= 1.0) {
            std::cout << "FPS: " << frameCount << std::endl;
            frameCount = 0;
            lastFpsTime = frameEnd;
        }
       
    }
}
}