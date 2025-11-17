#include "PrefabSystem.h"
#include <algorithm>
#include <cmath>  // наверху файла, если ещё нет
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "graphics/TextureCache.h"

namespace fs = std::filesystem;
namespace engine::data {

namespace {

// Генерация точек огня для "дерева" по bounds и scale
void GenerateTreeFirePointsFromBounds(const AgentPrefab& prefab, const glm::vec3& position,
                                      float scale, FireCollider& c) {
  const glm::vec3 scaled_min = prefab.m_bounds_min * scale;
  const glm::vec3 scaled_max = prefab.m_bounds_max * scale;

  const glm::vec3 world_min = position + scaled_min;
  const glm::vec3 world_max = position + scaled_max;

  const float height = scaled_max.y - scaled_min.y;
  if (height <= 0.0f) {
    // вырожденный объём – одна точка в центре
    const glm::vec3 center = 0.5f * (world_min + world_max);
    c.point_count = 1;
    c.heat_points[0] = Float3::fromGlm(center);
    c.fuels[0] = 100.0f;
    c.intensities[0] = 0.0f;
    return;
  }

  c.point_count = 0;
  constexpr int MAX_POINTS = 64;

  // --- ствол: несколько точек по высоте в центре
  const int trunkSegments = 6;
  for (int i = 0; i < trunkSegments && c.point_count < MAX_POINTS; ++i) {
    const float t = (i + 0.5f) / trunkSegments;  // 0..1
    const float yLocal = scaled_min.y + t * height;
    glm::vec3 world_pos = position + glm::vec3(0.0f, yLocal, 0.0f);

    c.heat_points[c.point_count] = Float3::fromGlm(world_pos);
    c.fuels[c.point_count] = 30.0f;
    c.intensities[c.point_count] = 0.0f;
    ++c.point_count;
  }

  // --- крона: несколько колец по радиусу
  const float radiusX = 0.5f * (scaled_max.x - scaled_min.x);
  const float radiusZ = 0.5f * (scaled_max.z - scaled_min.z);
  const float baseRadius = std::max(radiusX, radiusZ);

  constexpr int crownLevels = 3;
  const float levelHeights[crownLevels] = {0.3f, 0.6f, 0.85f};  // доли высоты
  const float levelRadiusMul[crownLevels] = {0.4f, 0.8f, 0.6f};
  constexpr float PI = 3.1415926535f;

  for (int li = 0; li < crownLevels && c.point_count < MAX_POINTS; ++li) {
    const float hLocal = scaled_min.y + levelHeights[li] * height;
    const float r = baseRadius * levelRadiusMul[li];
    const int numPts = 8 + li * 4;  // 8, 12, 16

    for (int i = 0; i < numPts && c.point_count < MAX_POINTS; ++i) {
      const float a = 2.0f * PI * (static_cast<float>(i) / numPts);
      const float cx = std::cos(a) * r;
      const float cz = std::sin(a) * r;

      glm::vec3 world_pos = position + glm::vec3(cx, hLocal, cz);

      c.heat_points[c.point_count] = Float3::fromGlm(world_pos);
      c.fuels[c.point_count] = 20.0f;
      c.intensities[c.point_count] = 0.0f;
      ++c.point_count;
    }
  }

  // на всякий случай – если что-то пошло не так
  if (c.point_count == 0) {
    const glm::vec3 center = 0.5f * (world_min + world_max);
    c.point_count = 1;
    c.heat_points[0] = Float3::fromGlm(center);
    c.fuels[0] = 100.0f;
    c.intensities[0] = 0.0f;
  }
}

}  // namespace

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
    throw std::runtime_error("Prefab not found:3 " + prefab_name);
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
    throw std::runtime_error("Prefab not found:2 " + name);
  }
  return it->second;
}
bool PrefabSystem::PrefabExists(const std::string& name) const {
  return m_prefabs.contains(name);
}

Agent PrefabSystem::CreateAgentFromPrefab(const std::string& prefab_name, const glm::vec3& position,
                                          float scale) {
  const AgentPrefab& prefab = GetPrefab(prefab_name);
  return CreateAgentFromPrefab(prefab, position, scale);
}

Agent PrefabSystem::CreateAgentFromPrefab(const AgentPrefab& prefab, const glm::vec3& position,
                                          float scale) {
  Agent agent{};
  agent.position = Float3::fromGlm(position);
  agent.type = prefab.m_type;
  agent.intensity = prefab.m_physics.m_intencity;

  FireCollider& c = agent.fire_collider;

  // 1) Масштабируем bounds относительно позиции
  glm::vec3 scaled_min = prefab.m_bounds_min * scale;
  glm::vec3 scaled_max = prefab.m_bounds_max * scale;

  c.bounds_min = Float3::fromGlm(position + scaled_min);
  c.bounds_max = Float3::fromGlm(position + scaled_max);

  // 2) Если в префабе явно заданы fire_points – используем их
  if (!prefab.m_firePoints.empty()) {
    c.point_count = std::min<int>(prefab.m_firePoints.size(), 64);

    for (int i = 0; i < c.point_count; ++i) {
      const auto& fp = prefab.m_firePoints[i];

      glm::vec3 scaled_local_pos = fp.m_position * scale;
      glm::vec3 world_pos = position + scaled_local_pos;

      c.heat_points[i] = Float3::fromGlm(world_pos);
      c.fuels[i] = fp.m_initialFuel;
      c.intensities[i] = 0.0f;
    }

    return agent;
  }

  // 3) Иначе – автогенерация по типу

  // Тип 0 в UI – "Tree", в enum это твой NORMAL
  if (prefab.m_type == AgentType::NORMAL) {
    GenerateTreeFirePointsFromBounds(prefab, position, scale, c);
  } else {
    // простой fallback: одна точка в центре объёма
    const glm::vec3 world_min = position + scaled_min;
    const glm::vec3 world_max = position + scaled_max;
    const glm::vec3 center = 0.5f * (world_min + world_max);

    c.point_count = 1;
    c.heat_points[0] = Float3::fromGlm(center);
    c.fuels[0] = 100.0f;
    c.intensities[0] = 0.0f;
  }

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

  prefab.m_physics.m_intencity = agent.intensity;

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

  // 🔥 fire
  {
    YAML::Node fire;
    fire["material"] = prefab.m_fireMaterialId;
    fire["ignition_mult"] = prefab.m_fireIgnitionMult;
    fire["humidity_offset"] = prefab.m_fireMoistureOffset;
    fire["moisture_offset"] = prefab.m_fireMoistureOffset;
    fire["fuel_mult"] = prefab.m_fireFuelMult;
    node["fire"] = fire;
  }

  // fire_points
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

  // bounds_min / bounds_max
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

  // physics.intensity
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

  // 🔥 fire: секция для физических параметров огня — максимально безопасно
  prefab.m_fireMaterialId = "wood";
  prefab.m_fireIgnitionMult = 1.0F;
  prefab.m_fireMoistureOffset = 0.0F;
  prefab.m_fireFuelMult = 1.0F;

  if (auto fire = node["fire"]; fire && fire.IsMap()) {
    if (auto mat = fire["material"]) {
      prefab.m_fireMaterialId = mat.as<std::string>();
    }
    if (auto ign_mult = fire["ignition_mult"]) {
      prefab.m_fireIgnitionMult = ign_mult.as<float>();
    }

    // moisture_offset: сначала новое имя, потом — старый humidity_offset для совместимости
    if (auto moist_off = fire["moisture_offset"]) {
      prefab.m_fireMoistureOffset = moist_off.as<float>();
    } else if (auto hum_off = fire["humidity_offset"]) {
      prefab.m_fireMoistureOffset = hum_off.as<float>();
    }

    // fuel_mult (опционален, если нет — останется 1.0)
    if (auto fuel_mult = fire["fuel_mult"]) {
      prefab.m_fireFuelMult = fuel_mult.as<float>();
    }
  }

  // materials
  if (node["materials"]["normal"]) {
    prefab.normal_material = ParseMaterialInfo(node["materials"]["normal"]);
  }
  if (node["materials"]["burning"]) {
    prefab.burning_material = ParseMaterialInfo(node["materials"]["burning"]);
  }
  if (node["materials"]["burned"]) {
    prefab.burned_material = ParseMaterialInfo(node["materials"]["burned"]);
  }

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
