#pragma once
#include "Shader.h"
#include <glm/vec3.hpp>
#include <memory>
#include <core/agent.h>
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

  void RenderFire(const std::vector<Agent>& agents, float dt);
  void EndScene();
private:
  void UpdateFireParticles(float dt, const std::vector<Agent>& burningAgents);
  void RenderFireParticles();
  glm::vec4 m_clearColor;
  glm::mat4 m_projectionMat;
  glm::mat4 m_viewMat;

  std::unique_ptr<Shader> m_shader;
  std::shared_ptr<Shader> m_modelShader;
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
      float maxLifetime;
      float size;
  };
  
  std::vector<FireParticle> m_fireParticles;
  std::vector<glm::mat4> m_particleModels;
  std::vector<glm::vec3> m_particleColors;
};
} // namespace engine