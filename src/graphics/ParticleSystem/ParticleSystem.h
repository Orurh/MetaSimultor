#pragma once
#include "ParticleEffect.h"
#include "graphics/ComputeShader.h"
#include "graphics/Shader.h"
#include "graphics/ShaderStorageBuffer.h"
#include "graphics/Texture.h"
#include "graphics/VertexArrayObject.h"
#include "graphics/VertexBuffer.h"
#include <memory>

namespace engine {

class ParticleSystem {

public:
  ParticleSystem();
  ~ParticleSystem();
  struct FireInstance {
    glm::vec3 position;
    float intensity;
    glm::vec3 _padding;
  };

  void Play();
  void Stop();
  void Update();
  void Render(const glm::mat4 &view, const glm::mat4 &projection);
  void SetEffect(const std::shared_ptr<ParticleEffect> &effect);
  void SetFireEffect();
  bool IsPlaying() const { return m_playing; }
  uint32_t GetParticleCount() const { return m_aliveCount; }
  auto GetEffect() const { return m_effect; }
  void SetTexture(const std::shared_ptr<Texture> &texture) {
    m_particleTexture = texture;
  }

  void LoadTexture(const std::string &path) {
    m_particleTexture = Texture::Create(path);
  }

  void SetPosition(const glm::vec3 &position) { m_position = position; }
  void UpdateInstances(const std::vector<FireInstance> &instances);
  void RenderInstanced(const glm::mat4 &view, const glm::mat4 &projection);
  void SetWind(const glm::vec3 &wind) { m_wind = wind; }

private:
  struct GPUParticle {
    glm::vec4 position; // xyz + lifetime
    glm::vec4 velocity; // xyz + startLifetime
    glm::vec4 color;
    glm::vec4 size;
  };

  void InitializeBuffers();
  void InitializeRenderData();
  void DispatchComputeShader();
  void CompactParticles();
  uint32_t CalculateEmitCount();
  std::shared_ptr<ParticleEffect> m_effect;

  // Shaders
  std::shared_ptr<ComputeShader> m_emitCS;
  std::shared_ptr<ComputeShader> m_updateCS;
  std::shared_ptr<ComputeShader> m_compactCS;
  std::shared_ptr<Shader> m_renderShader;

  // Buffers
  std::shared_ptr<ShaderStorageBuffer> m_particleBuffer;
  std::shared_ptr<ShaderStorageBuffer> m_counterBuffer;
  std::shared_ptr<ShaderStorageBuffer> m_instanceBuffer;
  uint32_t m_maxInstances = 1000000;
  uint32_t m_activeInstances = 0;
  // Render
  std::shared_ptr<VertexArrayObject> m_vao;
  std::shared_ptr<VertexBuffer> m_vbo;
  std::shared_ptr<Texture> m_particleTexture;

  glm::vec3 m_position;
  glm::vec3 m_wind = glm::vec3(0.0f);

  // State
  static constexpr uint32_t MAX_PARTICLES = 10000;
  static constexpr uint32_t WORK_GROUP_SIZE = 64;

  uint32_t m_aliveCount = 0;
  bool m_playing = false;
  float m_time = 0.0f;
  float m_emitTimer = 0.0f;
};

} // namespace engine