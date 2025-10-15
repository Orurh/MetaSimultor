#pragma once
#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include "core/agent.h"
#include "graphics/Material.h"
namespace engine::data {

struct AgentPrefab {
  std::string m_name;
  AgentType m_type = AgentType::NORMAL;

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

  struct MaterialInfo {
    std::string diffuse_path;
    std::string specular_path;
    std::string normal_path;
    glm::vec3 diffuse_color = glm::vec3(1.0f, 0.0f, 1.0f);
    glm::vec3 specular_color = glm::vec3(0.5f);
    float shininess = 32.0f;

    // Создает материал из этих путей
    std::shared_ptr<Material> CreateMaterial() const {
      return Material::Create(diffuse_path, specular_path, normal_path, diffuse_color,
                              specular_color, shininess);
    }
  };

  MaterialInfo normal_material;
  MaterialInfo burning_material;
  MaterialInfo burned_material;

  std::shared_ptr<Material> normalMaterial;
  std::shared_ptr<Material> burningMaterial;
  std::shared_ptr<Material> burnedMaterial;

  void LoadMaterials() {
    normalMaterial = normal_material.CreateMaterial();
    burningMaterial = burning_material.CreateMaterial();
    burnedMaterial = burned_material.CreateMaterial();
  }
  std::shared_ptr<Material> GetMaterialForState(AgentType state) const {
    switch (state) {
      case AgentType::BURNED:
        return burnedMaterial;
      case AgentType::BURNING:
        return burningMaterial;
      default:
        return normalMaterial;
    }
  }
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
  
  AgentPrefab& GetPrefabMutable(const std::string& name) {
    auto it = m_prefabs.find(name);
    if (it == m_prefabs.end())
        throw std::runtime_error("Prefab not found: " + name);
    return it->second;
}

 private:
  AgentPrefab::MaterialInfo ParseMaterialInfo(const YAML::Node& node) const {
    AgentPrefab::MaterialInfo info;

    if (node["diffuse_path"]) {
      info.diffuse_path = node["diffuse_path"].as<std::string>();
    }
    if (node["specular_path"]) {
      info.specular_path = node["specular_path"].as<std::string>();
    }
    if (node["normal_path"]) {
      info.normal_path = node["normal_path"].as<std::string>();
    }
    if (node["diffuse_color"]) {
      info.diffuse_color = ParseVec3(node["diffuse_color"], glm::vec3(0.8f));
    }
    if (node["specular_color"]) {
      info.specular_color = ParseVec3(node["specular_color"], glm::vec3(0.5f));
    }
    if (node["shininess"]) {
      info.shininess = node["shininess"].as<float>(32.0f);
    }

    return info;
  }

void SerializeMaterialInfo(const AgentPrefab::MaterialInfo& info, YAML::Node& node) const {
    node["diffuse_path"]  = info.diffuse_path;
    node["specular_path"] = info.specular_path;
    node["normal_path"]   = info.normal_path;

    // inline массив для цвета
    YAML::Node diffuseNode(YAML::NodeType::Sequence);
    diffuseNode.push_back(info.diffuse_color.x);
    diffuseNode.push_back(info.diffuse_color.y);
    diffuseNode.push_back(info.diffuse_color.z);
    diffuseNode.SetStyle(YAML::EmitterStyle::Flow);
    node["diffuse_color"] = diffuseNode;

    YAML::Node specularNode(YAML::NodeType::Sequence);
    specularNode.push_back(info.specular_color.x);
    specularNode.push_back(info.specular_color.y);
    specularNode.push_back(info.specular_color.z);
    specularNode.SetStyle(YAML::EmitterStyle::Flow);
    node["specular_color"] = specularNode;

    node["shininess"] = info.shininess;
}


//   void SerializeMaterialInfo(const AgentPrefab::MaterialInfo& info, YAML::Node& node) const {
//     node["diffuse_path"] = info.diffuse_path;
//     node["specular_path"] = info.specular_path;
//     node["normal_path"] = info.normal_path;

//     node["diffuse_color"] = YAML::Node(YAML::NodeType::Sequence);
//     node["diffuse_color"].push_back(info.diffuse_color.x);
//     node["diffuse_color"].push_back(info.diffuse_color.y);
//     node["diffuse_color"].push_back(info.diffuse_color.z);

//     node["specular_color"] = YAML::Node(YAML::NodeType::Sequence);
//     node["specular_color"].push_back(info.specular_color.x);
//     node["specular_color"].push_back(info.specular_color.y);
//     node["specular_color"].push_back(info.specular_color.z);

//     node["shininess"] = info.shininess;
//   }
  static glm::vec3 ParseVec3(const YAML::Node& node,
                             const glm::vec3& default_value = glm::vec3(0.0f)) {
    if (!node || !node.IsSequence() || node.size() != 3) {
      return default_value;
    }

    try {
      return glm::vec3(node[0].as<float>(), node[1].as<float>(), node[2].as<float>());
    } catch (...) {
      return default_value;
    }
  }
  void SerializePrefab(const AgentPrefab& prefab, YAML::Node& node) const;

  AgentPrefab DeserializePrefab(const YAML::Node& node) const;
  std::unordered_map<std::string, AgentPrefab> m_prefabs;

  std::vector<std::string> m_idsInLoadOrder;
};
}  // namespace engine::data