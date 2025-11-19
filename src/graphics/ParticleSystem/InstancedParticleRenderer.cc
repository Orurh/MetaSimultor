// InstancedParticleRenderer.cpp
#include "InstancedParticleRenderer.h"
#include "graphics/ResourceCache.h"
#include <glad/glad.h>

namespace engine {

InstancedParticleRenderer::InstancedParticleRenderer() = default;

InstancedParticleRenderer::~InstancedParticleRenderer() { Shutdown(); }

void InstancedParticleRenderer::Initialize() {
  m_shader = ResourceCache::GetShader("res/shaders/particle_instanced.sh");
  m_maxInstances = 100000;  // 100k частиц

  // Создаем SSBO для инстанс данных
  std::vector<ParticleInstanceData> initialData(m_maxInstances);
  m_instanceSSBO =
    ShaderStorageBuffer::Create(m_maxInstances * sizeof(ParticleInstanceData), initialData.data(), GL_DYNAMIC_DRAW);

  SetupGeometry();
  //   m_shader = ResourceCache::GetShader("res/shaders/particle_instanced.sh");
  //   SetupGeometry();
  //   SetupInstanceBuffer();
}

void InstancedParticleRenderer::SetupGeometry() {
  // Quad vertices (такие же как раньше)
  float vertices[] = {-0.5f, 0.5f, 0.0f, 1.0f, -0.5f, -0.5f, 0.0f, 0.0f, 0.5f, -0.5f, 1.0f, 0.0f,

                      -0.5f, 0.5f, 0.0f, 1.0f, 0.5f,  -0.5f, 1.0f, 0.0f, 0.5f, 0.5f,  1.0f, 1.0f};

  m_quadVBO = VertexBuffer::Create(vertices, sizeof(vertices));
  m_quadVBO->SetLayout({{ShaderDataType::Float2, "a_position"}, {ShaderDataType::Float2, "a_texcoord"}});

  m_vao = VertexArrayObject::Create();
  m_vao->AddVertexBuffer(m_quadVBO);
}

void InstancedParticleRenderer::SetupInstanceBuffer() {
  // Буфер для инстанс данных
  m_instanceVBO = VertexBuffer::Create(nullptr, MAX_INSTANCES * sizeof(ParticleInstanceData));
  m_instanceVBO->SetLayout({{ShaderDataType::Float3, "a_worldPosition", 1},  // инстанс атрибут
                            {ShaderDataType::Float4, "a_particleData", 1},
                            {ShaderDataType::Float4, "a_color", 1}});

  m_vao->AddVertexBuffer(m_instanceVBO);

  // Устанавливаем делитель для инстанс атрибутов
  for (uint32_t i = 1; i <= 3; i++) {
    glEnableVertexAttribArray(i);
    glVertexAttribDivisor(i, 1);
  }
}
void InstancedParticleRenderer::Render(const glm::mat4 &view, const glm::mat4 &projection,
                                       const std::vector<ParticleSystem *> &systems) {
  if (systems.empty())
    return;

  // 1. Собираем ВСЕ частицы ВСЕХ систем
  std::vector<ParticleInstanceData> allParticles;
  uint32_t totalParticles = 0;

  for (auto *system : systems) {
    uint32_t aliveCount = system->GetAliveCount();
    if (aliveCount == 0)
      continue;

    auto particles = system->GetParticlesData();
    glm::vec3 systemPos = system->GetPosition();

    for (uint32_t i = 0; i < aliveCount; i++) {
      ParticleInstanceData instance;
      instance.worldPosition = systemPos;
      instance.particleData =
        glm::vec4(particles[i].position.x, particles[i].position.y, particles[i].position.z, particles[i].size.x);
      instance.color = particles[i].color;
      allParticles.push_back(instance);
    }

    totalParticles += aliveCount;
    if (totalParticles >= m_maxInstances)
      break;
  }

  if (totalParticles == 0)
    return;

  std::cout << "TRUE INSTANCING: " << totalParticles << " particles from " << systems.size() << " systems" << std::endl;

  // 2. Загружаем ВСЕ данные в ОДИН SSBO
  m_instanceSSBO->SetData(allParticles.data(), totalParticles * sizeof(ParticleInstanceData));

  // 3. ОДИН draw call на ВСЕ системы!
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_FALSE);

  m_shader->Bind();
  m_shader->SetMat4("u_view", view);
  m_shader->SetMat4("u_projection", projection);
  m_shader->SetInt("u_useTexture", 1);
  m_shader->SetInt("u_instanceCount", totalParticles);  // Сколько инстансов рендерить

  if (systems[0]->GetTexture()) {
    systems[0]->GetTexture()->Bind(0);
    m_shader->SetInt("u_particleTexture", 0);
  }

  // Биндим ОДИН SSBO со всеми частицами
  m_instanceSSBO->Bind(0);

  m_vao->Bind();
  glDrawArraysInstanced(GL_TRIANGLES, 0, 6, totalParticles);  // ОДИН draw call!
  m_vao->Unbind();

  m_shader->Unbind();
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
}
void InstancedParticleRenderer::Shutdown() {
  m_vao.reset();
  m_quadVBO.reset();
  m_instanceVBO.reset();
}

}  // namespace engine