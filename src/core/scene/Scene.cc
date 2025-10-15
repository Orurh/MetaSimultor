#include "Scene.h"
#include <cuda_runtime.h>
#include <imgui.h>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>  // glm::perspective
#include <iostream>
#include "Entity.h"
#include "core/Input.h"
#include "core/Window.h"
#include "core/agent.h"
#include "core/simulation.cuh"
namespace engine {
Scene::Scene() : m_agentManager(1000000), m_camera(glm::vec3(500.0f, 100.0f, 600.0f)) {}

void Scene::Init() {
  GetPrefabSystem().LoadAllFromDirectory("res/prefabs");
  m_renderer.Init();
}

void Scene::EnsureModelsForPrefab(const std::string& id, const engine::data::AgentPrefab& prefab) {
  if (m_objectModels.count(id)) {
    return;
  }

  const std::string& path = prefab.m_modelPath;
  if (path.empty()) {
    std::cerr << "[Scene] prefab '" << id << "' has empty model_path\n";
    return;
  }

  // Если у тебя нет отдельных путей для normal/burning/burned — грузим одну и ту же модель.
  LoadObjectModels(id,
                   path,  // normal
                   path,  // burning
                   path   // burned
  );
}

void Scene::Update(float dt) {
  auto startTime = std::chrono::high_resolution_clock::now();

  m_windTime += dt;

  auto simStart = std::chrono::high_resolution_clock::now();
  if (!m_sceneObjects.empty()) {
    SimulationStep(m_agentManager.getDeviceAgents(), m_agentManager.getAgentCount(), dt,
                   m_windConfig.getCurrentWind(m_windTime));
  }
  auto simTime = std::chrono::high_resolution_clock::now();
  //  std::cout << "sim time" <<  std::chrono::duration_cast<std::chrono::milliseconds>(simTime - startTime).count() << std::endl;

  const bool UI_MOUSE = engine::InputSystem::Instance().UIMouseCaptured();
  const bool UI_KBD = engine::InputSystem::Instance().UIKeyboardCaptured();

  // 2) WASD — только если UI не держит клавиатуру
  if (!UI_KBD) {
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
  }

  // 3) Поворот камеры — только если UI не держит мышь И зажат RMB
  const glm::vec2 DELTA = engine::InputSystem::Instance().GetMouseDelta();
  if (!UI_MOUSE && engine::InputSystem::Instance().GetMouseButton(engine::MouseButton::Right,
                                                                  InputState::Held)) {
    if (DELTA.x != 0.F || DELTA.y != 0.F) {
      m_camera.ProcessMouseMovement(DELTA.x, DELTA.y);
    }
  }

  // 5) Зум — только если UI не держит мышь/скролл
  if (!UI_MOUSE) {
    const float SCROLL_Y = engine::InputSystem::Instance().GetMouseScrollY();
    if (SCROLL_Y != 0.0F) {
      m_camera.ProcessMouseScroll(SCROLL_Y);
      engine::InputSystem::Instance().ResetMouseScroll();
    }
  }
}

void Scene::Render() {
  const int FB_W = static_cast<int>(m_vpW * m_vpScaleX);
  const int FB_H = static_cast<int>(m_vpH * m_vpScaleY);
  glViewport(0, 0, FB_W, FB_H);

  const float ASPECT = FB_H > 0 ? static_cast<float>(FB_W) / static_cast<float>(FB_H) : 1.0F;
  auto projection = glm::perspective(m_camera.GetZoom(), ASPECT, 0.1F, 2000.0F);

  auto view = m_camera.GetViewMatrix();
  m_renderer.BeginScene(view, projection);

  auto agents = LoadAgentsFromGPU();

  // Рендеринг объектов
  for (const auto& obj : m_sceneObjects) {
    if (!obj.is_visible) {
      continue;
    }

    if (obj.agent_id < agents.size()) {
      const Agent& agent = agents[obj.agent_id];
      RenderAgent(agent, obj);
    }
  }

  m_renderer.UploadAgentsData(agents);

  m_renderer.RenderWithGeometryShader(m_windConfig.getCurrentWind(m_windTime));

  m_renderer.EndScene();
}

void Scene::Generate() {
  for (int i = 0; i < 50; i += 3) {
    for (int j = 0; j < 50; j += 3) {
      AddTree(500 + i, 500 + j, "default_tree");
    }
  }
}

std::vector<Agent> Scene::LoadAgentsFromGPU() {
  size_t currentCount = m_agentManager.getAgentCount();
  std::vector<Agent> agents(currentCount);

  cudaMemcpy(agents.data(), m_agentManager.getDeviceAgents(), currentCount * sizeof(Agent),
             cudaMemcpyDeviceToHost);

  // Проверка ошибок CUDA
  cudaError_t err = cudaGetLastError();
  if (err != cudaSuccess) {
    printf("CUDA memcpy error: %s\n", cudaGetErrorString(err));
  }

  return agents;
}
void Scene::LoadObjectModels(const std::string& object_name, const std::string& normal_path,
                             const std::string& burning_path, const std::string& burned_path) {
  try {
    ObjectModels models;
    models.normal = std::make_shared<Model>(normal_path);
    models.burning = std::make_shared<Model>(burning_path);
    models.burned = std::make_shared<Model>(burned_path);
    m_objectModels[object_name] = models;
  } catch (const std::exception& e) {
    std::cout << "Failed to load model '" << object_name << "': " << e.what() << std::endl;
  }
}

void Scene::RenderAgent(const Agent& agent, const SceneObject& scene_obj) {
  glm::vec3 position(agent.position.x, agent.position.y, agent.position.z);

  // Получаем префаб
  const auto& prefab = m_prefabSystem.GetPrefab(scene_obj.object_type);

  // Получаем модель
  auto it = m_objectModels.find(scene_obj.object_type);
  if (it == m_objectModels.end() || !it->second.normal) {
    std::cerr << "[RenderAgent] no model for '" << scene_obj.object_type << "'\n";
    return;
  }

  // Определяем состояние агента
  AgentType state = AgentType::NORMAL;
  if (agent.type == AgentType::BURNED) {
    state = AgentType::BURNED;
  } else if (agent.isBurning() || agent.type == AgentType::BURNING) {
    state = AgentType::BURNING;
  }

  auto material = prefab.GetMaterialForState(state);
  auto model = it->second.normal;

  if (model && material) {
    m_renderer.DrawModelWithMaterial(*model, position, scene_obj.scale, m_camera.GetPosition(),
                                     material);
  } else {
    // Fallback
    m_renderer.DrawModel(*model, position, scene_obj.scale, m_camera.GetPosition());
  }
}

glm::mat4 Scene::GetProjMatrix(int w, int h) {
  return glm::perspective(m_camera.GetZoom(), static_cast<float>(w) / static_cast<float>(h), 0.1f,
                          2000.0f);
}

void Scene::Clear() {
  m_sceneObjects.clear();
  m_agentManager.Clear();
}

// Model* Scene::GetModelForAgent(const Agent& agent, const std::string& object_type) {
//     auto it = m_objectModels.find(object_type);
//     if (it == m_objectModels.end()) return nullptr;

//     ObjectModels& models = it->second;

//     if (agent.type == AgentType::BURNED) {
//         return models.burned.get();
//     } else if (agent.isBurning() || agent.type == AgentType::BURNING) {
//         return models.burning.get();
//     } else {
//         return models.normal.get();
//     }
// }

void Scene::AddPrefab(const std::string& prefab_id, float x, float z) {
  const auto& prefab = m_prefabSystem.GetPrefab(prefab_id);

  // 1) гарантируем, что модели для этого prefab загружены
  EnsureModelsForPrefab(prefab_id, prefab);

  // 2) создаём агента
  Agent agent = m_prefabSystem.CreateAgentFromPrefab(prefab, {x, 0, z});
  m_agentManager.AddAgent(agent);

  // 3) объект сцены
  SceneObject obj;
  obj.agent_id = m_agentManager.getAgentCount() - 1;
  obj.object_type = prefab_id;  // ключ в m_objectModels
  obj.position = glm::vec3(x, 0, z);
  obj.scale = prefab.m_modelScale;
  obj.is_visible = true;
  m_sceneObjects.push_back(obj);
}

// Если нужно, оставь AddTree как шорткат:
void Scene::AddTree(float x, float z, const std::string& tree_type) {
  AddPrefab(tree_type, x, z);
}

// Список id префабов (проксируем)
const std::vector<std::string>& Scene::ListPrefabIds() const {
  return m_prefabSystem.ListIds();
}

void Scene::LoadMaterials() {
  // Материалы для деревьев
  m_treeMaterials["normal"] =
      Material::Create("res/models/tree/tree_diffuse.png", "res/models/tree/tree_specular.png",
                       "res/models/tree/tree_normal.png");

  m_treeMaterials["burning"] = Material::Create(
      "res/models/tree/tree_burning_diffuse.png", "res/models/tree/tree_burning_specular.png",
      "res/models/tree/tree_normal.png", glm::vec3(1.0f, 0.3f, 0.1f), glm::vec3(1.0f, 0.5f, 0.2f));

  m_treeMaterials["burned"] = Material::Create(
      "res/models/tree/tree_burned_diffuse.png", "", "res/models/tree/tree_normal.png",
      glm::vec3(0.2f, 0.2f, 0.2f), glm::vec3(0.1f, 0.1f, 0.1f));

  m_houseMaterials["normal"] =
      Material::Create("res/models/house/house_diffuse.png", "res/models/house/house_specular.png");
}

void Scene::ReloadModelForPrefab(const std::string& prefab_id,
                                 const engine::data::AgentPrefab& prefab) {
  // Перезагрузка модели
  ObjectModels models;
  models.normal = std::make_shared<Model>(prefab.m_modelPath);
  models.burning = std::make_shared<Model>(prefab.m_modelPath);
  models.burned = std::make_shared<Model>(prefab.m_modelPath);
  m_objectModels[prefab_id] = std::move(models);

  // Обновляем префаб в системе (чтобы GetMaterialForState возвращал свежий материал)
  auto& system_prefab = m_prefabSystem.GetPrefabMutable(prefab_id);
  system_prefab.normalMaterial = prefab.normalMaterial;
  system_prefab.burningMaterial = prefab.burningMaterial;
  system_prefab.burnedMaterial = prefab.burnedMaterial;

  // Обновляем scale для объектов сцены
  for (auto& obj : m_sceneObjects) {
    if (obj.object_type == prefab_id) {
      obj.scale = prefab.m_modelScale;
    }
  }

  std::cout << "[Scene] Reloaded model for prefab '" << prefab_id << "'\n";
}

// привязываем модель к префабу (если нужно или не нужно)
void Scene::AddSceneObjectFromSerialized(const SceneObject& obj) {
  const auto& prefab = m_prefabSystem.GetPrefab(obj.object_type);
  EnsureModelsForPrefab(obj.object_type, prefab);

  // Установка масштаба из префаба, если нужно
  SceneObject o = obj;
  o.scale = prefab.m_modelScale;

  m_sceneObjects.push_back(o);
}

}  // namespace engine
