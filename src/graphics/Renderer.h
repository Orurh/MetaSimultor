#pragma once
#include "Shader.h"
#include <core/agent.h>
#include <glm/vec3.hpp>
#include <memory>
#include <unordered_map>

#include "Material.h"
#include "Model.h"
#include "Texture.h"
#include "configs/FireParams.h"
#include "graphics/VertexArrayObject.h"
namespace engine {
struct AgentRenderData {
  glm::vec3 position;
  float intensity;
  glm::vec3 bounds_min;
  glm::vec3 bounds_max;
};
class Renderer {
public:
  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  static Renderer &Instance() {
    static Renderer instance;
    return instance;
  }
  void Init();
  void Shutdown();
  void DrawCube(const glm::vec3 &position, const glm::vec3 &color,
                const glm::vec3 &size);
  void DrawModel(Model &model, const glm::vec3 &position,
                 const glm::vec3 &scale, const glm::vec3 &camPos);
  void DrawModelInstanced(Model &model, const std::vector<glm::mat4> &instances,
                          const glm::vec3 &camPos,
                          std::shared_ptr<Material> material);
  void DrawModelWithMaterial(Model &model, const glm::vec3 &position,
                             const glm::vec3 &scale, const glm::vec3 &camPos,
                             const std::shared_ptr<Material> &material);
  void BeginScene(const glm::mat4 &view, const glm::mat4 &projection);
  void UploadAgentsData(const std::vector<Agent> &agents);
  void EndScene();
  void DrawTerrain(std::shared_ptr<VertexArrayObject> vao,
                   const glm::vec3 &terrainPos,
                   std::shared_ptr<Texture> terrainTexture);
  void SetFireParams(const FireParams &params);
  void SetEffectParams(const std::string &uniformName,
                       const FireParams::EffectParams &params);

private:
  Renderer() = default;
  struct UniversalParticleData {
    glm::vec3 position;
    float intensity;
    glm::vec3 bounds_min;
    glm::vec3 bounds_max;
    float effect_type; // 0=fire, 1=smoke, 2=sparks, 3=ash, 4=heat
  };
  std::shared_ptr<Shader> m_universalParticleShader;
  std::vector<UniversalParticleData> m_particleData;
  std::shared_ptr<VertexBuffer> m_particleVBO;
  std::shared_ptr<VertexArrayObject> m_particleVAO;
  FireParams m_fireParams;

  glm::vec4 m_clearColor;
  glm::mat4 m_projectionMat;
  glm::mat4 m_viewMat;

  // shaders
  std::unique_ptr<Shader> m_shader;
  std::shared_ptr<Shader> m_modelShader;
  std::shared_ptr<Shader> m_fireGeometryShader;
  std::shared_ptr<Shader> m_instanceModelShader;
  std::shared_ptr<Shader> m_terrainShader;

  // fire particles
  std::shared_ptr<Texture> m_fireTexture;
  std::shared_ptr<Texture> m_smokeTexture;
  std::shared_ptr<Texture> m_sparksTexture;
  std::shared_ptr<Texture> m_ashTexture;
  std::shared_ptr<Texture> m_heatTexture;

  std::vector<AgentRenderData> m_agentRenderData;
  std::shared_ptr<VertexBuffer> m_agentVBO;
  std::shared_ptr<VertexArrayObject> m_agentVAO;

  // cubes
  std::shared_ptr<VertexArrayObject> m_vao;
  std::shared_ptr<VertexBuffer> m_vbo;
  std::shared_ptr<IndexBuffer> m_ebo;
  std::shared_ptr<VertexBuffer> m_instanceVBO;
  std::shared_ptr<VertexArrayObject> m_instanceVAO;
  std::unordered_map<Model *, std::vector<glm::mat4>> m_instanceBatch;
};
} // namespace engine