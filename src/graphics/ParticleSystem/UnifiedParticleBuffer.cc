#include "UnifiedParticleBuffer.h"
#include "graphics/ResourceCache.h"
#include <cstdint>

namespace engine::graphics {
UnifiedParticleBuffer::UnifiedParticleBuffer() : m_totalAliveCount(0) {
  std::vector<GPUParticle> particles(MAX_PARTICLES);
  for (int i = 0; i < MAX_PARTICLES; i++) {
    particles[i].position = glm::vec4(0.0f, 0.0f, 0.0f, -1.0f);
    particles[i].velocity = glm::vec4(0.0f);
    particles[i].color = glm::vec4(0.0f);
    particles[i].size = glm::vec4(0.0f);
  }

  m_particleSSBO = ShaderStorageBuffer::Create(sizeof(GPUParticle) * MAX_PARTICLES, particles.data(), GL_DYNAMIC_COPY);

  std::vector<uint32_t> counters(MAX_PARTICLES / WORK_GROUP_SIZE + 1, 0);
  m_counterSSBO = ShaderStorageBuffer::Create(sizeof(uint32_t) * counters.size(), counters.data(), GL_DYNAMIC_COPY);
  m_allocatedBlocks.resize(MAX_PARTICLES / WORK_GROUP_SIZE, false);

  // Загружаем compute shaders
  m_emitCS = ResourceCache::GetComputeShader("res/shaders/particle_emit_unified.cs");
  m_updateCS = ResourceCache::GetComputeShader("res/shaders/particle_update_unified.cs");
  m_compactCS = ResourceCache::GetComputeShader("res/shaders/particle_compact_unified.cs");
}
UnifiedParticleBuffer::~UnifiedParticleBuffer() {}

UnifiedParticleBuffer::SystemAllocation UnifiedParticleBuffer::AllocateSystem(uint32_t maxParticles) {
  if (maxParticles == 0 || maxParticles > MAX_PARTICLES) {
    return {0, 0, 0, 0};
  }

  uint32_t alignedSize = ((maxParticles + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE) * WORK_GROUP_SIZE;

  for (uint32_t i = 0; i < m_allocatedBlocks.size(); i++) {
    if (!m_allocatedBlocks[i]) {
      uint32_t blockOffset = i * WORK_GROUP_SIZE;

      if (blockOffset + alignedSize <= MAX_PARTICLES) {
        m_allocatedBlocks[i] = true;

        SystemAllocation allocation;
        allocation.offset = blockOffset;
        allocation.capacity = alignedSize;
        allocation.aliveCount = 0;
        allocation.systemID = i;

        return allocation;
      }
    }
  }

  return {0, 0, 0, 0};
}

void UnifiedParticleBuffer::FreeSystem(const SystemAllocation &allocation) {
  if (allocation.systemID < m_allocatedBlocks.size()) {
    m_allocatedBlocks[allocation.systemID] = false;
    std::vector<GPUParticle> toCleanParticles(allocation.capacity);

    for (uint32_t i = 0; i < allocation.capacity; ++i) {
      toCleanParticles[i].position = glm::vec4(0, 0, 0, -1);
    }

    m_particleSSBO->SetData(toCleanParticles.data(), allocation.capacity * sizeof(GPUParticle),
                            allocation.offset * sizeof(GPUParticle));
  }
}

void UnifiedParticleBuffer::Bind(uint32_t bindingPoint) { m_particleSSBO->Bind(bindingPoint); }
void UnifiedParticleBuffer::DispatchEmit(uint32_t systemID, uint32_t emitCount, const glm::vec3 &position) {
  if (!m_emitCS || emitCount == 0)
    return;

  m_emitCS->Bind();

  // Устанавливаем параметры эмиттера
  m_emitCS->SetUInt("u_systemID", systemID);
  m_emitCS->SetUInt("u_emitCount", emitCount);
  m_emitCS->SetFloat3("u_emitterPosition", position);

  // Привязываем буферы
  m_particleSSBO->Bind(0);
  m_counterSSBO->Bind(1);

  // Диспетчеризация
  uint32_t groups = (emitCount + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE;
  m_emitCS->Dispatch(groups, 1, 1);
  m_emitCS->MemoryBarrier();

  UpdateCounters();
}

void UnifiedParticleBuffer::DispatchUpdate(float dt) {
  if (!m_updateCS || m_totalAliveCount == 0)
    return;

  m_updateCS->Bind();

  m_updateCS->SetFloat("u_deltaTime", dt);
  m_updateCS->SetUInt("u_totalAliveCount", m_totalAliveCount);

  m_particleSSBO->Bind(0);
  m_counterSSBO->Bind(1);

  uint32_t groups = (m_totalAliveCount + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE;
  m_updateCS->Dispatch(groups, 1, 1);
  m_updateCS->MemoryBarrier();

  UpdateCounters();
}

void UnifiedParticleBuffer::DispatchCompact() {
  if (!m_compactCS || m_totalAliveCount == 0)
    return;

  m_compactCS->Bind();

  m_particleSSBO->Bind(0);
  m_counterSSBO->Bind(1);

  uint32_t groups = (m_totalAliveCount + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE;
  m_compactCS->Dispatch(groups, 1, 1);
  m_compactCS->MemoryBarrier();

  UpdateCounters();
}
}  // namespace engine::graphics