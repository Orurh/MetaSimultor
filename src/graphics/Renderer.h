#pragma once
#include "Shader.h"
#include <glm/vec3.hpp>
#include <memory>
#include <core/agent.h>
#include <unordered_map>

#include "Texture.h"
#include "Model.h"
#include "Material.h"
namespace engine {
   struct AgentRenderData {
    glm::vec3 position;
    float intensity;
    glm::vec3 bounds_min;
    glm::vec3 bounds_max;
  };  
class Renderer {
public:
  void Init();
  void Shutdown();
  void DrawCube(const glm::vec3& position, const glm::vec3& color, float scale);
  void DrawModel(Model &model, const glm::vec3 &position, const glm::vec3& scale, const glm::vec3& camPos);
  void DrawModelInstanced(Model& model, const std::vector<glm::mat4>& instances, 
                          const glm::vec3& camPos);
  void DrawModelWithMaterial(Model& model, 
                                    const glm::vec3& position,
                                    const glm::vec3& scale, 
                                    const glm::vec3& camPos,
                                    const std::shared_ptr<Material>& material); 
  void BeginScene(const glm::mat4& view, const glm::mat4& projection);
  void UploadAgentsData(const std::vector<Agent>& agents);
  void RenderWithGeometryShader(const glm::vec3& wind_force);
  void EndScene();
private:
  glm::vec4 m_clearColor;
  glm::mat4 m_projectionMat;
  glm::mat4 m_viewMat;
 
  //shaders
  std::unique_ptr<Shader> m_shader;
  std::shared_ptr<Shader> m_modelShader;
  std::shared_ptr<Shader> m_fireGeometryShader;
  std::shared_ptr<Shader> m_instanceModelShader;

  //fire particles
  std::shared_ptr<Texture> m_fireTexture;
  std::vector<AgentRenderData> m_agentRenderData;
  std::shared_ptr<VertexBuffer> m_agentVBO;
  std::shared_ptr<VertexArrayObject> m_agentVAO;

  //cubes
  std::shared_ptr<VertexArrayObject> m_vao;
  std::shared_ptr<VertexBuffer> m_vbo;
  std::shared_ptr<IndexBuffer> m_ebo;
  std::shared_ptr<VertexBuffer> m_instanceBuffer;
    std::shared_ptr<VertexArrayObject> m_instanceVAO;
  std::unordered_map<Model*, std::vector<glm::mat4>> m_instanceBatch;
};
} // namespace engine