#include "Scene.h"
#include "Entity.h"
#include "core/simulation.cuh"
#include <cuda_runtime.h>
#include <iostream>
#include "core/Input.h"
#include "core/Window.h"
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

    AddFire(510, 500);
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
        if (mouseDelta != glm::vec2(0.0f)) {
            m_camera.ProcessMouseMovement(mouseDelta.x, mouseDelta.y);
        }
    }
    float scrollY = engine::InputSystem::Instance().GetMouseScrollY();
    if (scrollY != 0.0f) {
        m_camera.ProcessMouseScroll(scrollY);
        engine::InputSystem::Instance().ResetMouseScroll();
    }
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
        if ((agent.type == AgentType::FIRE_SOURCE || agent.type == AgentType::BURNED) ) {
            burningBranches.push_back(agent);
        }
    }
    
    if (burningBranches.empty()) return;
    m_renderer.DrawAgents(burningBranches);
    m_renderer.RenderFire(agents, 0.016f); 
    m_renderer.EndScene();
}

void Scene::AddTree(float x, float y)
{
  static int tree_counter = 0;
  m_agentManager.AddTree(tree_counter++, x, y);
}

void Scene::Generate()
{
    for(int i = 0; i < 5; i+=5){
        for(int j = 0; j < 5; j+=5) {
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
    glm::vec3 position(agent.pos_x, agent.pos_y, agent.pos_z);
    
    switch (agent.type) {
        case AgentType::TREE:
            m_renderer.DrawModel(*m_treeModel, position, m_camera.GetPosition());
            break;
       
    }
}
}
