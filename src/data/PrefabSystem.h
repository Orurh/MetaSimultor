#pragma once
#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include "core/agent.h"

namespace engine::data {

struct AgentPrefab {
  std::string m_name;
  AgentType m_type = AgentType::TREE;

  glm::vec3 m_modelScale = glm::vec3(1.0F);

  std::string m_modelPath;
  glm::vec3 m_bounds_min = {-1.0F, 0.0F, -1.0F};
  glm::vec3 m_bounds_max = {1.0F, 2.0F, 1.0F};

  struct FirePoint {
    glm::vec3 m_position;
    float m_initialFuel = 100.0F;
  };
  std::vector<FirePoint> m_firePoints;

  struct Phisics {
    float m_intencity;
  } m_physics;

  struct Visuals {
    std::string m_normal_material;
    std::string m_burning_material;
    std::string m_burned_material;
  } m_visuals;
};

class PrefabSystem {
 public:
  void SavePrefab(const std::string& name, const AgentPrefab& prefab);
  void SavePrefabToFile(const std::string& prefab_name, const std::string& file_path);
  void LoadPrefabFromFile(const std::string& file_path);
  const AgentPrefab& GetPrefab(const std::string& name) const;
  bool PrefabExists(const std::string& name) const;

  Agent CreateAgentFromPrefab(const std::string& prefab_name, const glm::vec3& position);
  Agent CreateAgentFromPrefab(const AgentPrefab& prefab, const glm::vec3& position);
  void CreatePrefabFromAgent(const std::string& name, const Agent& agent,
                             const std::string& model_path,
                             const glm::vec3& scale = glm::vec3(1.0F));

  std::vector<std::string> GetPrefabNames() const;
  std::vector<std::string> GetPrefabsByType(AgentType type) const;
  const std::vector<std::string>& ListIds() const { return m_idsInLoadOrder; }

  void LoadAllFromDirectory(const std::string& dir);
  void Clear();

 private:
  void SerializePrefab(const AgentPrefab& prefab, YAML::Node& node) const;

  AgentPrefab DeserializePrefab(const YAML::Node& node) const;
  std::unordered_map<std::string, AgentPrefab> m_prefabs;

  std::vector<std::string> m_idsInLoadOrder;
};
}  // namespace engine::data