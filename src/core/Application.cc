// Application.cc
#include "Application.h"

#include <stdexcept>
#include <thread>
#include <iostream>

// GLAD → GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// ImGui core + backends
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

namespace engine {
Application::Application(int width, int height, const std::string &title)
  : m_window(width, height, title)
{
    // GLAD после создания контекста
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }
    glViewport(0, 0, width, height);

    // --- ImGui init (минимум) ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    // опциональные флаги:
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    // backends
    GLFWwindow* win = m_window.GetWindow(); // или твой геттер
    ImGui_ImplGlfw_InitForOpenGL(win, /*install_callbacks=*/true);
    ImGui_ImplOpenGL3_Init("#version 330"); // при желании можно "#version 130"

    m_scene.Init();
}

void Application::Run()
{
    const double TARGET_FPS  = 60.0;
    const double FRAME_TIME  = 1.0 / TARGET_FPS;

    while(!m_window.ShouldClose()) {
        double frameStart = glfwGetTime();

        float currentFrame = static_cast<float>(glfwGetTime());
        float dt = currentFrame - m_last_frame_time;
        m_last_frame_time = currentFrame;

        m_window.PollEvents();

        // --- логика/рендер твоей сцены (костры и т.д.) ---
        m_scene.Update(dt);
        m_scene.Render();

        // --- ImGui frame begin ---
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- Мини-окно: загрузка модели (пока без деревьев) ---
        ImGui::Begin("Model Loader");
        static char path[512] = "res/models/LowPolytree/CartoonTree.obj";
        ImGui::InputText("Path", path, IM_ARRAYSIZE(path));
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            m_scene.LoadPrimaryModel(path); // добавь метод в Scene, см. ниже
        }
        ImGui::TextUnformatted("Formats: OBJ/FBX/DAE/etc. via Assimp");
        ImGui::End();

        // Можно быстро проверить, что ImGui рисуется:
        // static bool show_demo = true; ImGui::ShowDemoWindow(&show_demo);

        // --- ImGui render ---
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        m_window.SwapBuffers();

        // ограничение FPS (опционально)
        // double frameEnd = glfwGetTime();
        // double frameDuration = frameEnd - frameStart;
        // if (frameDuration < FRAME_TIME) {
        //     std::this_thread::sleep_for(std::chrono::duration<double>(FRAME_TIME - frameDuration));
        // }
    }

    // --- Shutdown ImGui ---
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
} // namespace engine