// UnifiedParticleBuffer.h
#pragma once
#include "graphics/ComputeShader.h"
#include "graphics/ShaderStorageBuffer.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
namespace engine::graphics {
struct GPUParticle {
  glm::vec4 position;  // xyz + lifetime
  glm::vec4 velocity;  // xyz + startLifetime
  glm::vec4 color;
  glm::vec4 size;
};

class UnifiedParticleBuffer {
  public:
  static constexpr uint32_t MAX_PARTICLES = 1000000;
  static constexpr uint32_t WORK_GROUP_SIZE = 64;

  UnifiedParticleBuffer();
  ~UnifiedParticleBuffer();

  struct SystemAllocation {
    uint32_t offset;
    uint32_t capacity;
    uint32_t aliveCount;
    uint32_t systemID;
  };

  SystemAllocation AllocateSystem(uint32_t maxParticles);
  void FreeSystem(const SystemAllocation &allocation);

  void Bind(uint32_t bindingPoint = 0);
  uint32_t GetTotalAliveCount() const { return m_totalAliveCount; };
  void DispatchEmit(uint32_t systemID, uint32_t emitCount, const glm::vec3 &position);
  void DispatchUpdate(float dt);
  void DispatchCompact();

  private:
  void UpdateCounters() {
    // Читаем общее количество живых частиц из буфера счетчиков
    // В этом упрощенном примере считаем, что первый элемент содержит общее количество
    uint32_t totalAlive = 0;
    m_counterSSBO->GetData(&totalAlive, sizeof(totalAlive), 0);
    m_totalAliveCount = totalAlive;
  }
  std::shared_ptr<ShaderStorageBuffer> m_particleSSBO;
  std::shared_ptr<ShaderStorageBuffer> m_counterSSBO;
  std::vector<bool> m_allocatedBlocks;
  uint32_t m_totalAliveCount;

  std::shared_ptr<ComputeShader> m_emitCS;
  std::shared_ptr<ComputeShader> m_updateCS;
  std::shared_ptr<ComputeShader> m_compactCS;
};
}  // namespace engine::graphics