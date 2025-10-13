// Application.cc
#include "Application.h"

#include <stdexcept>

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

    ImGui::StyleColorsDark();

    // backends
    GLFWwindow* win = m_window.GetWindow(); 
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

        // --- Мини-окно: загрузка модели ---
        ImGui::Begin("Model Loader");
        static char path[512] = "res/models/LowPolytree/CartoonTree.obj";
        ImGui::InputText("Path", path, IM_ARRAYSIZE(path));
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            m_scene.LoadPrimaryModel(path); // добавь метод в Scene, см. ниже
        }
        ImGui::TextUnformatted("Formats: OBJ/FBX/DAE/etc. via Assimp");
        ImGui::End();

       // --- Режим редактирования кликом ---
        ImGui::Begin("Edit Mode");
        static int mode = 0; // 0=Tree, 1=Fire
        ImGui::RadioButton("Add Tree (LMB)", &mode, 0); ImGui::SameLine();
        ImGui::RadioButton("Add Fire (LMB)", &mode, 1);
        if (mode == 0) {
            m_scene.SetPlaceMode(engine::Scene::PlaceMode::Tree);
        } else {
            m_scene.SetPlaceMode(engine::Scene::PlaceMode::Fire);
            static float fireIntensity = 1.0f;
            ImGui::SliderFloat("Fire intensity", &fireIntensity, 0.0f, 5.0f, "%.2f");
            m_scene.SetFireIntensity(fireIntensity);
        }
        ImGui::TextUnformatted("ЛКМ по земле: добавить выбранное");
        ImGui::End();


        // --- Панель управления огнем ---
        ImGui::Begin("Fire Control");
        static float fx = 510.0f;
        static float fy = 501.0f;
        static float fint = 1.0f;
        ImGui::InputFloat("X", &fx);
        ImGui::InputFloat("Y", &fy);
        ImGui::SliderFloat("Intensity", &fint, 0.0f, 5.0f, "%.2f");
        if (ImGui::Button("Ignite")) {
            m_scene.AddFire(fx, fy, fint);
        }
        ImGui::End();


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

void Application::SaveScene(const std::string& path) {
    if (!m_serializer) {
        m_serializer = std::make_unique<data::SceneSerializer>(
            std::make_shared<Scene>(m_scene));
    }
    m_serializer->Serialize(path);
} 
void Application::LoadScene(const std::string& path) {
    if (!m_serializer) {
        m_serializer = std::make_unique<data::SceneSerializer>(
           std::make_shared<Scene>(m_scene));
    }
    m_serializer->Deserialize(path);
}
} // namespace engine