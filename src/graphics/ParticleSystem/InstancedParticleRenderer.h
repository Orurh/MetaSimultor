// InstancedParticleRenderer.h
#pragma once
#include "ParticleSystem.h"
#include "graphics/Shader.h"
#include "graphics/VertexArrayObject.h"
#include "graphics/VertexBuffer.h"
#include <vector>

namespace engine {

class InstancedParticleRenderer {
  public:
  InstancedParticleRenderer();
  ~InstancedParticleRenderer();

  void Initialize();
  void Render(const glm::mat4 &view, const glm::mat4 &projection, const std::vector<ParticleSystem *> &systems);
  void Shutdown();

  private:
  void SetupGeometry();
  void SetupInstanceBuffer();
  std::shared_ptr<ShaderStorageBuffer> m_instanceSSBO;

  std::shared_ptr<Shader> m_shader;
  std::shared_ptr<VertexArrayObject> m_vao;
  std::shared_ptr<VertexBuffer> m_quadVBO;
  std::shared_ptr<VertexBuffer> m_instanceVBO;
  uint32_t m_maxInstances;

  static constexpr uint32_t MAX_INSTANCES = 100000;  // 100k частиц максимум
};

}  // namespace engine