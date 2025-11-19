#include "ParticleSystem.h"
#include "graphics/ResourceCache.h"
#include <chrono>
#include <cstring>
#include <iostream>
namespace Time {
static float GetDeltaTime() { return 0.016f; }
}  // namespace Time

namespace engine {

ParticleSystem::ParticleSystem() {

  m_emitCS = ResourceCache::GetComputeShader("res/shaders/particle_emit.cs");
  m_updateCS = ResourceCache::GetComputeShader("res/shaders/particle_update.cs");
  m_renderShader = ResourceCache::GetShader("res/shaders/particles.sh");
  m_compactCS = ResourceCache::GetComputeShader("res/shaders/particle_compact.cs");
  InitializeBuffers();
  InitializeRenderData();

  m_effect = ParticleEffect::CreateFire();
  m_playing = true;
}

ParticleSystem::~ParticleSystem() {}
void ParticleSystem::CompactParticles() {
  m_compactCS->Bind();
  m_particleBuffer->Bind(0);
  m_counterBuffer->Bind(1);

  uint32_t groups = (m_aliveCount + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE;
  m_compactCS->Dispatch(groups, 1, 1);
  m_compactCS->MemoryBarrier();

  // Обновляем счетчик
  uint32_t currentAlive;
  m_counterBuffer->GetData(&currentAlive, sizeof(currentAlive));
  m_aliveCount = currentAlive;
}

void ParticleSystem::InitializeBuffers() {

  std::vector<GPUParticle> particles(MAX_PARTICLES);
  for (int i = 0; i < MAX_PARTICLES; i++) {
    particles[i].position = glm::vec4(0.0f, 0.0f, 0.0f, -1.0f);
    particles[i].velocity = glm::vec4(0.0f);
    particles[i].color = glm::vec4(0.0f);
    particles[i].size = glm::vec4(0.0f);
  }

  m_particleBuffer =
    ShaderStorageBuffer::Create(sizeof(GPUParticle) * MAX_PARTICLES, particles.data(), GL_DYNAMIC_DRAW);

  uint32_t aliveCount = 0;
  m_counterBuffer = ShaderStorageBuffer::Create(sizeof(aliveCount), &aliveCount, GL_DYNAMIC_DRAW);

  m_aliveCount = 0;
}

void ParticleSystem::InitializeRenderData() {

  // Quad vertices
  float vertices[] = {-0.5f, 0.5f, 0.0f, 1.0f, -0.5f, -0.5f, 0.0f, 0.0f, 0.5f, -0.5f, 1.0f, 0.0f,

                      -0.5f, 0.5f, 0.0f, 1.0f, 0.5f,  -0.5f, 1.0f, 0.0f, 0.5f, 0.5f,  1.0f, 1.0f};

  m_vbo = VertexBuffer::Create(vertices, sizeof(vertices));
  m_vbo->SetLayout({{ShaderDataType::Float2, "a_position"}, {ShaderDataType::Float2, "a_texcoord"}});

  m_vao = VertexArrayObject::Create();
  m_vao->AddVertexBuffer(m_vbo);
}

uint32_t ParticleSystem::CalculateEmitCount() {
  if (!m_effect || !m_playing)
    return 0;

  if (!m_effect->main.looping && m_time >= m_effect->main.duration) {
    return 0;
  }

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
  uint32_t emitCount = CalculateEmitCount();
  if (emitCount == 0 && m_aliveCount == 0)
    return;

  if (emitCount > 0 && m_emitCS) {
    m_emitCS->Bind();
    m_emitCS->SetUInt("u_emitCount", emitCount);
    m_emitCS->SetFloat3("u_emitterPosition", m_position);

    if (m_effect) {
      m_emitCS->SetFloat3("u_startVelocity", m_effect->main.startSpeed);
      m_emitCS->SetFloat("u_startLifetime", m_effect->main.startLifetime);
      m_emitCS->SetFloat4("u_startColor", m_effect->main.startColor);
      m_emitCS->SetFloat3("u_startSize", m_effect->main.startSize);
      m_emitCS->SetFloat3("u_startRotation", m_effect->main.startRotation);
      m_emitCS->SetInt("u_use3DRotation", m_effect->main.use3DRotation ? 1 : 0);

      m_emitCS->SetInt("u_shapeType", m_effect->shape.shapeType);
      m_emitCS->SetFloat("u_shapeRadius", m_effect->shape.radius);
      m_emitCS->SetFloat("u_shapeAngle", m_effect->shape.angle);
      m_emitCS->SetFloat3("u_shapeBoxSize", m_effect->shape.boxSize);
      m_emitCS->SetFloat("u_shapeArc", m_effect->shape.arc);
      m_emitCS->SetFloat("u_shapeRandomDirection", m_effect->shape.randomDirection);
      m_emitCS->SetInt("u_shapeAlignToDirection", m_effect->shape.alignToDirection ? 1 : 0);
    }

    m_particleBuffer->Bind(0);
    m_counterBuffer->Bind(1);

    uint32_t groups = (emitCount + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE;
    m_emitCS->Dispatch(groups, 1, 1);
    m_emitCS->MemoryBarrier();

    m_aliveCount = std::min(m_aliveCount + emitCount, MAX_PARTICLES);
  }

  // 2. Update pass
  if (m_updateCS && m_aliveCount > 0) {
    m_updateCS->Bind();
    m_updateCS->SetFloat("u_deltaTime", Time::GetDeltaTime());
    m_updateCS->SetUInt("u_aliveCount", m_aliveCount);
    m_updateCS->SetFloat3("u_emitterPosition", m_position);
    m_updateCS->SetFloat("u_simulationSpeed", m_effect->main.simulationSpeed);
    m_updateCS->SetFloat3("u_windForce", m_wind);

    if (m_effect) {
      // Physics
      m_updateCS->SetFloat("u_gravity", m_effect->main.gravityModifier * 9.81f);

      // Lifetime modules
      m_updateCS->SetInt("u_sizeOverLifetimeEnabled", m_effect->sizeOverLifetime.enabled ? 1 : 0);
      m_updateCS->SetInt("u_colorOverLifetimeEnabled", m_effect->colorOverLifetime.enabled ? 1 : 0);

      // Color over lifetime gradient
      if (m_effect->colorOverLifetime.enabled && !m_effect->colorOverLifetime.gradient.empty()) {
        std::vector<glm::vec4> gradientColors(4);
        std::vector<float> gradientTimes(4);

        for (int i = 0; i < 4; i++) {
          if (i < m_effect->colorOverLifetime.gradient.size()) {
            gradientColors[i] = m_effect->colorOverLifetime.gradient[i].color;
            gradientTimes[i] = m_effect->colorOverLifetime.gradient[i].time;
          } else {
            gradientColors[i] = gradientColors.back();
            gradientTimes[i] = 1.0f;
          }
        }

        m_updateCS->SetFloat4Array("u_colorOverLifetimeGradient", gradientColors.data(), 4);
        m_updateCS->SetFloatArray("u_colorOverLifetimeTimes", gradientTimes.data(), 4);
      }

      // Size over lifetime curve
      if (m_effect->sizeOverLifetime.enabled && !m_effect->sizeOverLifetime.curve.empty()) {
        std::vector<glm::vec3> sizeCurve(4);
        std::vector<float> sizeTimes(4);

        for (int i = 0; i < 4; i++) {
          if (i < m_effect->sizeOverLifetime.curve.size()) {
            sizeCurve[i] = m_effect->sizeOverLifetime.curve[i].size;
            sizeTimes[i] = m_effect->sizeOverLifetime.curve[i].time;
          } else {
            sizeCurve[i] = sizeCurve.back();
            sizeTimes[i] = 1.0f;
          }
        }

        m_updateCS->SetFloat3Array("u_sizeOverLifetimeCurve", sizeCurve.data(), 4);
        m_updateCS->SetFloatArray("u_sizeOverLifetimeTimes", sizeTimes.data(), 4);
      }

      // Velocity over lifetime
      m_updateCS->SetInt("u_velocityOverLifetimeEnabled", m_effect->velocityOverLifetime.enabled ? 1 : 0);
      m_updateCS->SetFloat3("u_velocityOverLifetimeLinear", m_effect->velocityOverLifetime.linear);
      m_updateCS->SetFloat3("u_velocityOverLifetimeOrbital", m_effect->velocityOverLifetime.orbital);
      m_updateCS->SetFloat("u_velocityOverLifetimeSpeedModifier", m_effect->velocityOverLifetime.speedModifier);

      // Force over lifetime
      m_updateCS->SetInt("u_forceOverLifetimeEnabled", m_effect->forceOverLifetime.enabled ? 1 : 0);
      m_updateCS->SetFloat3("u_forceOverLifetime", m_effect->forceOverLifetime.force);

      // Limit velocity
      m_updateCS->SetInt("u_limitVelocityOverLifetimeEnabled", m_effect->limitVelocityOverLifetime.enabled ? 1 : 0);
      m_updateCS->SetFloat("u_limitVelocity", m_effect->limitVelocityOverLifetime.limit);
      m_updateCS->SetFloat("u_limitVelocityDampen", m_effect->limitVelocityOverLifetime.dampen);

      // Rotation over lifetime
      m_updateCS->SetInt("u_rotationOverLifetimeEnabled", m_effect->rotationOverLifetime.enabled ? 1 : 0);
      m_updateCS->SetFloat3("u_rotationOverLifetime", m_effect->rotationOverLifetime.angularVelocity);
    }

    m_particleBuffer->Bind(0);
    m_counterBuffer->Bind(1);

    uint32_t updateGroups = (m_aliveCount + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE;
    m_updateCS->Dispatch(updateGroups, 1, 1);
    m_updateCS->MemoryBarrier();

    // ⚡️ ПОСЛЕ UPDATE - УМЕНЬШИ СЧЕТЧИК ПРИБЛИЗИТЕЛЬНО
    m_aliveCount = m_aliveCount - 5;
  }
}

void ParticleSystem::Play() {
  m_playing = true;
  m_time = 0.0f;
  m_emitTimer = 0.0f;
}

void ParticleSystem::Stop() { m_playing = false; }

void ParticleSystem::Update() {
  float deltaTime = Time::GetDeltaTime();
  m_time += deltaTime;

  DispatchComputeShader();
}

void ParticleSystem::Render(const glm::mat4 &view, const glm::mat4 &projection) {
  if (!m_renderShader || m_aliveCount == 0 || !m_vao)
    return;
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_FALSE);
  m_renderShader->Bind();
  m_renderShader->SetMat4("u_view", view);
  m_renderShader->SetMat4("u_projection", projection);
  if (m_particleTexture) {
    m_particleTexture->Bind(0);
    m_renderShader->SetInt("u_particleTexture", 0);
    m_renderShader->SetInt("u_useTexture", 1);
  } else {
    m_renderShader->SetInt("u_useTexture", 0);
  }
  m_particleBuffer->Bind(0);
  m_vao->Bind();

  glDrawArraysInstanced(GL_TRIANGLES, 0, 6, m_aliveCount);
  m_vao->Unbind();

  // ДОБАВЬ ОТВЯЗКУ ТЕКСТУРЫ:
  glBindTexture(GL_TEXTURE_2D, 0);

  m_renderShader->Unbind();
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
}

void ParticleSystem::SetEffect(const std::shared_ptr<ParticleEffect> &effect) {
  m_effect = effect;

  // Сбрасываем систему при смене эффекта
  if (m_playing) {
    Stop();
    Play();
  }
}
void ParticleSystem::SetFireEffect() {
  auto effect = ResourceCache::GetParticleEffect("res/effects/Fire.yaml");
  SetEffect(effect);
  if (m_effect && !m_effect->renderer.texturePath.empty()) {
    m_particleTexture = ResourceCache::GetTexture(m_effect->renderer.texturePath);
  }
}

std::vector<ParticleInstanceData> ParticleSystem::GetInstanceData() const {
  std::vector<ParticleInstanceData> instances;
  if (m_aliveCount == 0)
    return instances;

  // Получаем данные частиц
  auto particles = GetParticlesData();
  instances.reserve(m_aliveCount);

  for (uint32_t i = 0; i < m_aliveCount; i++) {
    ParticleInstanceData instance;
    instance.worldPosition = m_position;
    instance.particleData =
      glm::vec4(particles[i].position.x, particles[i].position.y, particles[i].position.z, particles[i].size.x);
    instance.color = particles[i].color;
    instances.push_back(instance);
  }

  return instances;
}

}  // namespace engine