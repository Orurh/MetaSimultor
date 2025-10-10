#pragma once
#include "Shader.h"
#include <glm/vec3.hpp>
#include <memory>
#include <core/agent.h>
#include "Texture.h"
#include "Model.h"
namespace engine {
class Renderer {
public:
  void Init();
  void Shutdown();
  void DrawAgents(const std::vector<Agent>& agents);
  void DrawCube(const glm::vec3& position, const glm::vec3& color, float scale);
  void DrawModel(Model &model, const glm::vec3 &position, const glm::vec3& camPos);
  void BeginScene(const glm::mat4& view, const glm::mat4& projection);
  void UploadAgentsData(const std::vector<Agent>& agents);
  void RenderFire(const std::vector<Agent>& agents, float dt);
  void EndScene();
  void RenderWithGeometryShader();
private:
  void UpdateFireParticles(float dt, const std::vector<Agent>& burningAgents);
  void RenderFireParticles();
  glm::vec4 m_clearColor;
  glm::mat4 m_projectionMat;
  glm::mat4 m_viewMat;

  std::unique_ptr<Shader> m_shader;
  std::shared_ptr<Shader> m_modelShader;
  std::shared_ptr<Shader> m_fireGeometryShader;
  std::shared_ptr<Texture> m_fireTexture;
  std::shared_ptr<VertexArrayObject> m_vao;
  std::shared_ptr<VertexBuffer> m_vbo;
  std::shared_ptr<IndexBuffer> m_ebo;

  std::shared_ptr<VertexBuffer> m_instanceVBO;
  std::vector<glm::mat4> m_modelMatrices;
  std::vector<glm::vec3> m_colors;

    struct FireParticle {
        glm::vec3 position;
        glm::vec3 velocity;
        float lifetime;
        float max_lifetime;
        float size;
        glm::vec3 color;
    };

    struct AgentRenderData {
    glm::vec3 position;
    float intensity;
    glm::vec3 bounds_min;
    glm::vec3 bounds_max;
};  
  std::vector<AgentRenderData> m_agentRenderData;
  std::shared_ptr<VertexBuffer> m_agentVBO;
  std::shared_ptr<VertexArrayObject> m_agentVAO;

  std::vector<FireParticle> m_fireParticles;
  float m_particleSpawnTimer = 0.0f;
  std::vector<glm::mat4> m_particleModels;
  std::vector<glm::vec3> m_particleColors;
};
} // namespace engine