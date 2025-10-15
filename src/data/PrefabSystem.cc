#include "PrefabSystem.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace fs = std::filesystem;
namespace engine::data {

static void PushUnique(std::vector<std::string>& v, const std::string& id) {
  if (std::ranges::find(v, id) == v.end()) {
    v.push_back(id);
  }
}

void PrefabSystem::SavePrefab(const std::string& name, const AgentPrefab& prefab) {
  m_prefabs[name] = prefab;
  PushUnique(m_idsInLoadOrder, name);
}
void PrefabSystem::SavePrefabToFile(const std::string& prefab_name, const std::string& file_path) {
  if (!PrefabExists(prefab_name)) {
    throw std::runtime_error("Prefab not found: " + prefab_name);
  }

  const AgentPrefab& prefab = GetPrefab(prefab_name);

  YAML::Node root;
  root["name"] = prefab_name;
  SerializePrefab(prefab, root);

  fs::path path(file_path);
  fs::create_directories(path.parent_path());

  std::ofstream file(file_path);
  file << root;
  PushUnique(m_idsInLoadOrder, prefab_name);
  std::cout << "Saved prefab '" << prefab_name << "' to " << file_path << '\n';
}
void PrefabSystem::LoadPrefabFromFile(const std::string& file_path) {
  if (!fs::exists(file_path)) {
    throw std::runtime_error("File not found: " + file_path);
  }

  YAML::Node root = YAML::LoadFile(file_path);

  // name из YAML, иначе — из имени файла
  std::string name;
  if (auto n = root["name"]; n && n.IsScalar()) {
    name = n.as<std::string>();
  } else {
    name = fs::path(file_path).stem().string();
  }

  AgentPrefab prefab = DeserializePrefab(root);
  prefab.m_name = name;  // <<< ВАЖНО: заполняем поле структуры

  // защита от дубликатов (как минимум лог и замена/пропуск)
  auto it = m_prefabs.find(name);
  if (it != m_prefabs.end()) {
    std::cerr << "[PrefabSystem] duplicate name '" << name << "' from " << file_path
              << " — replacing previous.\n";
  }

  m_prefabs[name] = std::move(prefab);
  PushUnique(m_idsInLoadOrder, name);
  std::cout << "Loaded prefab '" << name << "' from " << file_path << '\n';
}

const AgentPrefab& PrefabSystem::GetPrefab(const std::string& name) const {
  auto it = m_prefabs.find(name);
  if (it == m_prefabs.end()) {
    throw std::runtime_error("Prefab not found: " + name);
  }
  return it->second;
}
bool PrefabSystem::PrefabExists(const std::string& name) const {
  return m_prefabs.contains(name);
}

Agent PrefabSystem::CreateAgentFromPrefab(const std::string& prefab_name,
                                          const glm::vec3& position) {
  // было: const AgentPrefab& prefab = m_prefabs[prefab_name]; // может создать пустую запись!
  const AgentPrefab& prefab = GetPrefab(prefab_name);  // бросит исключение, если нет
  return CreateAgentFromPrefab(prefab, position);
}

Agent PrefabSystem::CreateAgentFromPrefab(const AgentPrefab& prefab, const glm::vec3& position) {
  Agent agent;
  agent.position = Float3::fromGlm(position);
  agent.fire_collider.bounds_min = Float3::fromGlm(prefab.m_bounds_min + position);
  agent.fire_collider.bounds_max = Float3::fromGlm(prefab.m_bounds_max + position);

  agent.fire_collider.point_count = std::min(static_cast<int>(prefab.m_firePoints.size()), 64);

  for (int i = 0; i < agent.fire_collider.point_count; ++i) {
    Float3 local_pos = Float3::fromGlm(prefab.m_firePoints[i].m_position);
    agent.fire_collider.heat_points[i] =
        Float3(position.x + local_pos.x, position.y + local_pos.y, position.z + local_pos.z);
    agent.fire_collider.fuels[i] = prefab.m_firePoints[i].m_initialFuel;
    agent.fire_collider.intensities[i] = 0.0F;
  }

  agent.intencity = prefab.m_physics.m_intencity;
  agent.type = prefab.m_type;

  return agent;
}

void PrefabSystem::CreatePrefabFromAgent(const std::string& name, const Agent& agent,
                                         const std::string& model_path, const glm::vec3& scale) {
  AgentPrefab prefab;
  prefab.m_name = name;
  prefab.m_type = agent.type;
  prefab.m_modelPath = model_path;
  prefab.m_modelScale = scale;
  prefab.m_bounds_min = agent.fire_collider.bounds_min.toGlm();
  prefab.m_bounds_max = agent.fire_collider.bounds_max.toGlm();

  for (int i = 0; i < agent.fire_collider.point_count; i++) {
    AgentPrefab::FirePoint fp;
    fp.m_position = glm::vec3(agent.fire_collider.heat_points[i].x - agent.position.x,
                              agent.fire_collider.heat_points[i].y - agent.position.y,
                              agent.fire_collider.heat_points[i].z - agent.position.z);
    fp.m_initialFuel = agent.fire_collider.fuels[i];
    prefab.m_firePoints.push_back(fp);
  }

  prefab.m_physics.m_intencity = agent.intencity;

  m_prefabs[name] = prefab;
  PushUnique(m_idsInLoadOrder, name);
}

std::vector<std::string> PrefabSystem::GetPrefabNames() const {
  std::vector<std::string> result;
  result.reserve(m_prefabs.size());
  std::ranges::transform(m_prefabs, std::back_inserter(result),
                         [](const auto& pair) { return pair.first; });
  return result;
}
std::vector<std::string> PrefabSystem::GetPrefabsByType(AgentType type) const {
  std::vector<std::string> prefabs;

  for (auto [name, prefab] : m_prefabs) {
    if (prefab.m_type == type) {
      prefabs.push_back(name);
    }
  }

  return prefabs;
}
void PrefabSystem::SerializePrefab(const AgentPrefab& prefab, YAML::Node& node) const {
  node["type"] = static_cast<int>(prefab.m_type);
  node["model_path"] = prefab.m_modelPath;
  node["model_scale"] =
      std::vector<float>{prefab.m_modelScale.x, prefab.m_modelScale.y, prefab.m_modelScale.z};
  node["bounds_min"] =
      std::vector<float>{prefab.m_bounds_min.x, prefab.m_bounds_min.y, prefab.m_bounds_min.z};
  node["bounds_max"] =
      std::vector<float>{prefab.m_bounds_max.x, prefab.m_bounds_max.y, prefab.m_bounds_max.z};

  node["physics"]["intensity"] = prefab.m_physics.m_intencity;

  for (const auto& fp : prefab.m_firePoints) {
    YAML::Node fp_node;
    fp_node["position"] = std::vector<float>{fp.m_position.x, fp.m_position.y, fp.m_position.z};
    fp_node["initial_fuel"] = fp.m_initialFuel;
    node["fire_points"].push_back(fp_node);
  }
  YAML::Node normal_node;
  YAML::Node burning_node;
  YAML::Node burned_node;
  
  SerializeMaterialInfo(prefab.normal_material, normal_node);
  SerializeMaterialInfo(prefab.burning_material, burning_node);
  SerializeMaterialInfo(prefab.burned_material, burned_node);
  
  // ПРИСВАИВАЕМ созданные ноды
  node["materials"]["normal"] = normal_node;
  node["materials"]["burning"] = burning_node;
  node["materials"]["burned"] = burned_node;
}

// устойчивый к неполным файлам
AgentPrefab PrefabSystem::DeserializePrefab(const YAML::Node& node) const {
  AgentPrefab prefab;

  // type
  if (auto t = node["type"]) {
    prefab.m_type = static_cast<AgentType>(t.as<int>());
  } else {
    prefab.m_type = AgentType::NORMAL;
  }

  // model_path
  if (auto mp = node["model_path"]) {
    prefab.m_modelPath = mp.as<std::string>();
  } else {
    prefab.m_modelPath.clear();
  }

  // model_scale (опционален)
  if (auto ms = node["model_scale"]) {
    auto v = ms.as<std::vector<float>>();
    if (v.size() >= 3)
      prefab.m_modelScale = glm::vec3(v[0], v[1], v[2]);
    else
      prefab.m_modelScale = glm::vec3(1.0f);
  } else {
    prefab.m_modelScale = glm::vec3(1.0f);
  }

  // bounds_min / bounds_max (обязательные для твоей логики — но читаем бережно)
  if (auto bmin = node["bounds_min"]) {
    auto v = bmin.as<std::vector<float>>();
    if (v.size() >= 3)
      prefab.m_bounds_min = glm::vec3(v[0], v[1], v[2]);
  }
  if (auto bmax = node["bounds_max"]) {
    auto v = bmax.as<std::vector<float>>();
    if (v.size() >= 3)
      prefab.m_bounds_max = glm::vec3(v[0], v[1], v[2]);
  }

  // physics.intensity (может отсутствовать)
  if (auto phys = node["physics"]) {
    if (auto inten = phys["intensity"]) {
      prefab.m_physics.m_intencity = inten.as<float>();
    } else {
      prefab.m_physics.m_intencity = 0.0f;
    }
  } else {
    prefab.m_physics.m_intencity = 0.0f;
  }

  // fire_points (опциональны)
  prefab.m_firePoints.clear();
  if (auto fps = node["fire_points"]) {
    for (const auto& fp_node : fps) {
      AgentPrefab::FirePoint fp{};
      if (auto pos = fp_node["position"]) {
        auto v = pos.as<std::vector<float>>();
        if (v.size() >= 3)
          fp.m_position = glm::vec3(v[0], v[1], v[2]);
      }
      if (auto fuel = fp_node["initial_fuel"]) {
        fp.m_initialFuel = fuel.as<float>();
      } else {
        fp.m_initialFuel = 100.0f;
      }
      prefab.m_firePoints.push_back(fp);
    }
  }
    // Materials - ЗАПОЛНЯЕМ MaterialInfo
    if (node["materials"]["normal"]) {
        prefab.normal_material = ParseMaterialInfo(node["materials"]["normal"]);
    }
    if (node["materials"]["burning"]) {
        prefab.burning_material = ParseMaterialInfo(node["materials"]["burning"]);
    }
    if (node["materials"]["burned"]) {
        prefab.burned_material = ParseMaterialInfo(node["materials"]["burned"]);
    }
    
    // Загружаем текстуры
    prefab.LoadMaterials();

  return prefab;
}

void PrefabSystem::Clear() {
  m_prefabs.clear();
  m_idsInLoadOrder.clear();
}

void PrefabSystem::LoadAllFromDirectory(const std::string& dir) {
  namespace fs = std::filesystem;
  Clear();

  fs::path root(dir);
  if (!fs::exists(root))
    return;

  for (auto& e : fs::directory_iterator(root)) {
    if (!e.is_regular_file())
      continue;
    const auto ext = e.path().extension().string();
    if (ext != ".yaml" && ext != ".yml")
      continue;

    try {
      LoadPrefabFromFile(e.path().string());
    } catch (const std::exception& ex) {
      std::cerr << "[PrefabSystem] skip " << e.path() << " : " << ex.what() << "\n";
      continue;
    }
  }
}

}  // namespace engine::data
