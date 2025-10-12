#include "Scene.h"
#include "Entity.h"
#include "core/simulation.cuh"
#include <GLFW/glfw3.h>
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
    m_prefabSystem.LoadPrefabFromFile("prefabs/default_tree.yaml");
    Generate();

    // AddFire(510, 500);
}

void Scene::Update(float dt) {
  // 0) Симуляция — всегда идёт
  SimulationStep(m_agentManager.getDeviceAgents(),
                 m_agentManager.getAgentCount(), dt);

  // 1) Флаги захвата UI
  const ImGuiIO& io = ImGui::GetIO();
  const bool ui_mouse = io.WantCaptureMouse;
  const bool ui_kbd   = io.WantCaptureKeyboard;

  // 2) WASD — только если UI не держит клавиатуру
  if (!ui_kbd) {
    if (engine::InputSystem::Instance().GetKey(engine::KeyCode::W)) m_camera.ProcessKeyboard(FORWARD,  dt);
    if (engine::InputSystem::Instance().GetKey(engine::KeyCode::S)) m_camera.ProcessKeyboard(BACKWARD, dt);
    if (engine::InputSystem::Instance().GetKey(engine::KeyCode::D)) m_camera.ProcessKeyboard(RIGHT,    dt);
    if (engine::InputSystem::Instance().GetKey(engine::KeyCode::A)) m_camera.ProcessKeyboard(LEFT,     dt);
  }

  // 3) Поворот камеры — только если UI не держит мышь И зажат RMB
  static glm::vec2 lastMousePos = engine::InputSystem::Instance().GetMousePosition();
  const auto curMousePos = engine::InputSystem::Instance().GetMousePosition();

  if (!ui_mouse && engine::InputSystem::Instance()
                       .GetMouseButton(GLFW_MOUSE_BUTTON_RIGHT, InputState::Held)) {
    const glm::vec2 delta = curMousePos - lastMousePos;
    if (delta != glm::vec2(0.0f)) {
      m_camera.ProcessMouseMovement(delta.x, delta.y);
    }
  }
  // обновляем last всегда в конце кадра, чтобы дельта считалась корректно
  lastMousePos = curMousePos;

  // 4) Постановка объекта — ОДНОРАЗОВО на клик ЛКМ и только если UI не держит мышь
  if (!ui_mouse && engine::InputSystem::Instance()
                      .GetMouseButton(GLFW_MOUSE_BUTTON_LEFT, InputState::Pressed)) {
    const glm::vec2 mp = curMousePos;
    const glm::vec3 hit = ScreenToGround(mp, m_vpW, m_vpH);
    if (std::isfinite(hit.x) && std::isfinite(hit.z)) {
      if (m_placeMode == PlaceMode::Tree) {
        AddTree(hit.x, hit.z);
      } else {
        m_agentManager.AddFire(hit.x, hit.z, m_fireIntensity);
      }
    }
  }

  // 5) Зум — только если UI не держит мышь/скролл
  if (!ui_mouse) {
    const float scrollY = engine::InputSystem::Instance().GetMouseScrollY();
    if (scrollY != 0.0f) {
      m_camera.ProcessMouseScroll(scrollY);
      engine::InputSystem::Instance().ResetMouseScroll();
    }
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
    
     for (const auto& obj : m_sceneObjects) {
        if (!obj.is_visible) continue;
        
        if (obj.agent_id < agents.size()) {
            const Agent& agent = agents[obj.agent_id];
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
    
    const auto& prefab = m_prefabSystem.GetPrefab("default_tree");
    Agent agent = m_prefabSystem.CreateAgentFromPrefab(prefab, {x, 0, y});
    m_agentManager.AddAgent(agent);
    int agent_index = m_agentManager.getAgentCount() - 1;
    SceneObject obj;
    obj.agent_id = agent_index;
    obj.model_path = prefab.model_path; 
    obj.position = glm::vec3(x, 0, y);
    obj.is_visible = true;
    m_sceneObjects.push_back(obj);
    tree_counter++;
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
