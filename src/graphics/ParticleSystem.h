#pragma once
#include "ParticleEffect.h"
#include "graphics/ComputeShader.h"
#include "graphics/Shader.h"
#include "graphics/ShaderStorageBuffer.h"
#include "graphics/VertexArrayObject.h"
#include "graphics/VertexBuffer.h"
#include <memory>

namespace engine {

class ParticleSystem {
public:
  ParticleSystem();
  ~ParticleSystem();

  void Play();
  void Stop();
  void Update();
  void Render(const glm::mat4 &view, const glm::mat4 &projection);
  void SetEffect(const std::shared_ptr<ParticleEffect> &effect);
  bool IsPlaying() const { return m_playing; }
  uint32_t GetParticleCount() const { return m_aliveCount; }
  auto GetEffect() const { return m_effect; }

private:
  struct GPUParticle {
    glm::vec4 position; // xyz + lifetime
    glm::vec4 velocity; // xyz + startLifetime
    glm::vec4 color;
    glm::vec4 size;
    glm::uvec4 randomData;
  };

  void InitializeBuffers();
  void InitializeRenderData();
  void DispatchComputeShader();
  uint32_t CalculateEmitCount();
  std::shared_ptr<ParticleEffect> m_effect;

  // Shaders
  std::shared_ptr<ComputeShader> m_emitCS;
  std::shared_ptr<ComputeShader> m_updateCS;
  std::shared_ptr<Shader> m_renderShader;

  // Buffers
  std::shared_ptr<ShaderStorageBuffer> m_particleBuffer;
  std::shared_ptr<ShaderStorageBuffer> m_counterBuffer;

  // Render
  std::shared_ptr<VertexArrayObject> m_vao;
  std::shared_ptr<VertexBuffer> m_vbo;

  // State
  static constexpr uint32_t MAX_PARTICLES = 10000;
  static constexpr uint32_t WORK_GROUP_SIZE = 64;

  uint32_t m_aliveCount = 0;
  bool m_playing = false;
  float m_time = 0.0f;
  float m_emitTimer = 0.0f;
};

} // namespace engine