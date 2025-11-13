#include "ParticleSystem.h"
#include <cstring>
#include <iostream>

namespace Time {
static float GetDeltaTime() { return 0.016f; }
} // namespace Time

namespace engine {

ParticleSystem::ParticleSystem() {
  m_emitCS = ComputeShader::Create("res/shaders/particle_emit.cs");
  m_updateCS = ComputeShader::Create("res/shaders/particle_update.cs");
  m_renderShader = Shader::Create("res/shaders/particles.sh");

  InitializeBuffers();
  InitializeRenderData();

  m_effect = ParticleEffect::CreateFire();
  m_playing = true;
}

ParticleSystem::~ParticleSystem() {}

void ParticleSystem::InitializeBuffers() {
  // Particle buffer - все частицы мертвые изначально
  std::vector<GPUParticle> particles(MAX_PARTICLES);
  for (int i = 0; i < MAX_PARTICLES; i++) {
    particles[i].position = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f); // lifetime = 0
    particles[i].velocity = glm::vec4(0.0f);
    particles[i].color = glm::vec4(0.0f);
    particles[i].size = glm::vec4(0.0f);
    particles[i].randomData = glm::uvec4(0);
  }

  m_particleBuffer = ShaderStorageBuffer::Create(
      sizeof(GPUParticle) * MAX_PARTICLES, nullptr, GL_DYNAMIC_DRAW);

  // Counter buffer [aliveCount, deadCount, emitCount]
  uint32_t counters[3] = {0, MAX_PARTICLES, 0};
  m_counterBuffer =
      ShaderStorageBuffer::Create(sizeof(counters), counters, GL_DYNAMIC_DRAW);
}

void ParticleSystem::InitializeRenderData() {
  // Quad vertices
  float vertices[] = {-0.5f, 0.5f, 0.0f, 1.0f,  -0.5f, -0.5f,
                      0.0f,  0.0f, 0.5f, -0.5f, 1.0f,  0.0f,

                      -0.5f, 0.5f, 0.0f, 1.0f,  0.5f,  -0.5f,
                      1.0f,  0.0f, 0.5f, 0.5f,  1.0f,  1.0f};

  m_vbo = VertexBuffer::Create(vertices, sizeof(vertices));
  m_vbo->SetLayout({{ShaderDataType::Float2, "a_position"},
                    {ShaderDataType::Float2, "a_texcoord"}});

  m_vao = VertexArrayObject::Create();
  m_vao->AddVertexBuffer(m_vbo);
}

uint32_t ParticleSystem::CalculateEmitCount() {
  if (!m_effect || !m_playing)
    return 0;

  // Накопленное время для эмиссии
  m_emitTimer += Time::GetDeltaTime();
  float emissionInterval = 1.0f / m_effect->emission.rateOverTime;

  uint32_t emitCount = 0;
  while (m_emitTimer >= emissionInterval && emitCount < 100) {
    emitCount++;
    m_emitTimer -= emissionInterval;
  }

  return emitCount;
}

void ParticleSystem::DispatchComputeShader() {
  // 1. Emit pass
  uint32_t emitCount = CalculateEmitCount();
  if (emitCount > 0 && m_emitCS) {
    m_emitCS->Bind();
    m_emitCS->SetFloat("u_time", m_time);
    m_emitCS->SetFloat("u_deltaTime", Time::GetDeltaTime());
    m_emitCS->SetUInt("u_maxParticles", MAX_PARTICLES);
    m_emitCS->SetUInt("u_emitCount", emitCount);

    // Передаем параметры из эффекта
    if (m_effect) {
      // Main module
      m_emitCS->SetFloat("u_emissionRate", m_effect->emission.rateOverTime);
      m_emitCS->SetFloat3("u_startVelocity", m_effect->main.startSpeed);
      m_emitCS->SetFloat("u_startLifetime", m_effect->main.startLifetime);
      m_emitCS->SetFloat4("u_startColor", m_effect->main.startColor);
      m_emitCS->SetFloat3("u_startSize", m_effect->main.startSize);

      // Shape module
      // m_emitCS->SetFloat("u_shapeRadius", m_effect->shape.radius);
      // m_emitCS->SetFloat("u_shapeAngle", m_effect->shape.angle);
      // m_emitCS->SetFloat3("u_shapeBoxSize", m_effect->shape.boxSize);
      // m_emitCS->SetInt("u_shapeType", m_effect->shape.shapeType);
    }

    m_particleBuffer->Bind(0);
    m_counterBuffer->Bind(1);

    uint32_t groups = (emitCount + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE;
    m_emitCS->Dispatch(groups, 1, 1);
    m_emitCS->MemoryBarrier();
  }

  // 2. Update pass
  if (m_updateCS && m_aliveCount > 0) {
    m_updateCS->Bind();
    m_updateCS->SetFloat("u_time", m_time);
    m_updateCS->SetFloat("u_deltaTime", Time::GetDeltaTime());
    m_updateCS->SetUInt("u_aliveCount", m_aliveCount);

    if (m_effect) {
      // Physics
      m_updateCS->SetFloat("u_gravity", m_effect->main.gravityModifier * 9.81f);

      // Lifetime modules
      m_updateCS->SetBool("u_sizeOverLifetimeEnabled",
                          m_effect->sizeOverLifetime.enabled);
      m_updateCS->SetBool("u_colorOverLifetimeEnabled",
                          m_effect->colorOverLifetime.enabled);

      // Forces
      m_updateCS->SetFloat3("u_forceOverLifetime",
                            m_effect->forceOverLifetime.force);
      m_updateCS->SetFloat3("u_velocityOverLifetime",
                            m_effect->velocityOverLifetime.linear);
    }

    m_particleBuffer->Bind(0);
    m_counterBuffer->Bind(1);

    uint32_t updateGroups =
        (m_aliveCount + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE;
    m_updateCS->Dispatch(updateGroups, 1, 1);
    m_updateCS->MemoryBarrier();
  }
}

void ParticleSystem::Play() {
  m_playing = true;
  m_time = 0.0f;
  m_emitTimer = 0.0f;
}

void ParticleSystem::Stop() { m_playing = false; }

void ParticleSystem::Update() {
  if (!m_playing || !m_effect)
    return;

  float deltaTime = Time::GetDeltaTime();
  m_time += deltaTime;

  // Вызываем compute shaders
  DispatchComputeShader();

  // Обновляем alive count (в реальности читать из atomic counter)
  uint32_t emitCount = CalculateEmitCount();
  if (emitCount > 0) {
    m_aliveCount = std::min(m_aliveCount + emitCount, MAX_PARTICLES);
  }
}

void ParticleSystem::Render(const glm::mat4 &view,
                            const glm::mat4 &projection) {
  if (!m_renderShader || m_aliveCount == 0 || !m_vao)
    return;

  m_renderShader->Bind();
  m_renderShader->SetMat4("u_view", view);
  m_renderShader->SetMat4("u_projection", projection);

  m_particleBuffer->Bind(0);
  m_vao->Bind();

  glDrawArraysInstanced(GL_TRIANGLES, 0, 6, m_aliveCount);
  m_vao->Unbind();
}

void ParticleSystem::SetEffect(const std::shared_ptr<ParticleEffect> &effect) {
  m_effect = effect;

  // Сбрасываем систему при смене эффекта
  if (m_playing) {
    Stop();
    Play();
  }
}

} // namespace engine