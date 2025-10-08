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
  void EndScene();
private:
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
};
} // namespace engine