#include "Scene.h"
#include "Entity.h"
#include "core/simulation.cuh"
#include <cuda_runtime.h>
#include <iostream>
#include <chrono>
#include "core/Input.h"
#include "core/Window.h"
#include <imgui.h>
namespace engine {
Scene::Scene() : m_agentManager(1000000), m_camera(glm::vec3(500.0f, 100.0f, 600.0f))
{
}

void Scene::Init()
{
    m_treeModel = std::make_shared<Model>("res/models/LowPolytree/CartoonTree.obj");
    m_burningModel = std::make_shared<Model>("res/models/CartoonTree/CartoonTree.obj");
    m_burnedModel = std::make_shared<Model>("res/models/ChrTree/CartoonTree.obj");
    m_renderer.Init();
    Generate();

    // AddFire(510, 500);
}

void Scene::Update(float dt) {
    SimulationStep(m_agentManager.getDeviceAgents(), m_agentManager.getAgentCount(), dt);
    if (engine::InputSystem::Instance().GetKey(engine::KeyCode::W)) {
        m_camera.ProcessKeyboard(FORWARD, dt);
    }
    if (engine::InputSystem::Instance().GetKey(engine::KeyCode::S)) {
        m_camera.ProcessKeyboard(BACKWARD, dt);
    }
    if (engine::InputSystem::Instance().GetKey(engine::KeyCode::D)) {
        m_camera.ProcessKeyboard(RIGHT, dt);
    }
    if (engine::InputSystem::Instance().GetKey(engine::KeyCode::A)) {
        m_camera.ProcessKeyboard(LEFT, dt);
    }
    
    static glm::vec2 lastMousePos = engine::InputSystem::Instance().GetMousePosition();
    
    auto currentMousePos = engine::InputSystem::Instance().GetMousePosition();
    glm::vec2 mouseDelta = currentMousePos - lastMousePos;
    lastMousePos = currentMousePos;
    
    if (engine::InputSystem::Instance().GetMouseButton(GLFW_MOUSE_BUTTON_LEFT, InputState::Held)) {
        std::cout << "Held" << std::endl;
        if (mouseDelta != glm::vec2(0.0f)) {
            m_camera.ProcessMouseMovement(mouseDelta.x, mouseDelta.y);
        }
    }
    // Одноразовый клик для постановки объекта
    if (engine::InputSystem::Instance().GetMouseButton(GLFW_MOUSE_BUTTON_LEFT, InputState::Pressed)) {
        std::cout << "Pressed" << std::endl;
        bool ui_capture = false;
#ifdef USE_IMGUI
        ui_capture = ImGui::GetIO().WantCaptureMouse;
#endif
        if (!ui_capture) {
           glm::vec2 mp = engine::InputSystem::Instance().GetMousePosition();
            glm::vec3 p = ScreenToGround(mp, 1920, 1080);
            if (std::isfinite(p.x) && std::isfinite(p.z)) {
                if (m_placeMode == PlaceMode::Tree) {
                    AddTree(p.x, p.z);
                } else {
                    m_agentManager.AddFire(p.x, p.z, m_fireIntensity);
                }
            }
        }
    }
    float scrollY = engine::InputSystem::Instance().GetMouseScrollY();
    if (scrollY != 0.0f) {
        m_camera.ProcessMouseScroll(scrollY);
        engine::InputSystem::Instance().ResetMouseScroll();
    }
}

glm::vec3 Scene::ScreenToGround(const glm::vec2& mousePx, int w, int h) {
    float x =  2.0f * mousePx.x / float(w) - 1.0f;
    float y =  1.0f - 2.0f * mousePx.y / float(h);
    glm::mat4 proj = glm::perspective(m_camera.GetZoom(), float(w)/float(h), 0.1f, 2000.0f);
    glm::mat4 view = m_camera.GetViewMatrix();
    glm::mat4 invVP = glm::inverse(proj * view);
    glm::vec4 pNear = invVP * glm::vec4(x, y, -1.0f, 1.0f);
    glm::vec4 pFar  = invVP * glm::vec4(x, y,  1.0f, 1.0f);
    pNear /= pNear.w; pFar /= pFar.w;
    glm::vec3 origin = glm::vec3(pNear);
    glm::vec3 dir    = glm::normalize(glm::vec3(pFar - pNear));
    if (std::abs(dir.y) < 1e-6f) return glm::vec3(std::numeric_limits<float>::quiet_NaN());
    float t = -origin.y / dir.y;
    return origin + t * dir;
}

void Scene::Render() {
    auto projection = glm::perspective(m_camera.GetZoom(), (float)1920 / (float)1080, 0.1f, 2000.0f);
    auto view = m_camera.GetViewMatrix();
    m_renderer.BeginScene(view, projection);
    
    auto agents = LoadAgentsFromGPU(); 
    
    for(const auto& agent : agents) {
        if (agent.type != AgentType::FIRE_SOURCE) {  
            RenderAgent(agent);
        }
    }
    
    std::vector<Agent> burningBranches;
    for (const auto& agent : agents) {
        if ((agent.type == AgentType::TREE || agent.type == AgentType::BURNED)) {
            burningBranches.push_back(agent);
        }
    }
    m_renderer.UploadAgentsData(agents); 
    m_renderer.RenderWithGeometryShader();
    
    m_renderer.EndScene();
}

void Scene::AddTree(float x, float y)
{
  static int tree_counter = 0;
  m_agentManager.AddTree(tree_counter++, x, y);
//   m_agentManager.IgniteRandomPoint(0);
}

void Scene::Generate()
{
    for(int i = 0; i < 100; i+=3){
        for(int j = 0; j < 100; j+=3) {
            AddTree(500 + i, 500 + j);
        }
    }
}

std::vector<Agent> Scene::LoadAgentsFromGPU()
{
    std::vector<Agent> agents(m_agentManager.getAgentCount());
    cudaMemcpy(agents.data(), m_agentManager.getDeviceAgents(),
               agents.size() * sizeof(Agent), cudaMemcpyDeviceToHost);
    return agents;
}

void Scene::RenderAgent(const Agent &agent)
{
    glm::vec3 position(agent.position.x, agent.position.y, agent.position.z);
    
    switch (agent.type) {
        case AgentType::TREE:
            m_renderer.DrawModel(*m_treeModel, position, m_camera.GetPosition());
            break;
        case AgentType::BURNED:
            m_renderer.DrawModel(*m_burnedModel, position, m_camera.GetPosition());
            break;
    }   
}

void Scene::LoadPrimaryModel(const std::string& path) {
    m_treeModel = std::make_shared<Model>(path);
}

} // namespace engine
