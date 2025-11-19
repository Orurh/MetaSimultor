#include "AgentManager.h"
#include <cuda_runtime.h>
#include <iostream>
#include <print>
#include <random>
#include "core/agent.h"

namespace {
constexpr float FIRE_SOURCE_BASE_RADIUS = 0.5F;  // "радиус" источника огня по XZ
constexpr float FIRE_SOURCE_HEIGHT = 2.0F;       // высота по Y
}  // namespace

AgentManager::AgentManager(int max_agents) : m_capacity(max_agents), m_count(0) {
  cudaMalloc(&m_agents, max_agents * sizeof(Agent));
}

AgentManager::~AgentManager() {
  cudaFree(m_agents);
}

std::vector<Agent> AgentManager::LoadAgentsFromGPU() {
  size_t currentCount = GetAgentCount();
  std::vector<Agent> agents(currentCount);

  cudaMemcpy(agents.data(), GetDeviceAgents(), currentCount * sizeof(Agent),
             cudaMemcpyDeviceToHost);

  cudaError_t err = cudaGetLastError();
  if (err != cudaSuccess) {
    std::println("CUDA memcpy error: {}", cudaGetErrorString(err));
  }
  return agents;
}

void AgentManager::AddFireAt(const Float3& world, float intensity) {
  if (m_count >= m_capacity)
    return;

  Agent fire{};
  fire.type = FireAgentState::Burning;
  fire.object_id = -1;
  fire.position = world;

  fire.fire_collider.point_count = 1;
  fire.fire_collider.heat_points[0] = world;  // мировая точка источника
  fire.fire_collider.fuels[0] = 200.0F;
  fire.fire_collider.intensities[0] = intensity;

  const float HALF_HEIGHT = FIRE_SOURCE_HEIGHT * 0.5F;

  fire.fire_collider.bounds_min = Float3(world.x - FIRE_SOURCE_BASE_RADIUS, world.y - HALF_HEIGHT,
                                         world.z - FIRE_SOURCE_BASE_RADIUS);

  fire.fire_collider.bounds_max = Float3(world.x + FIRE_SOURCE_BASE_RADIUS, world.y + HALF_HEIGHT,
                                         world.z + FIRE_SOURCE_BASE_RADIUS);

  AddAgent(fire);
}

int AgentManager::AddAgent(const Agent& agent) {
  if (m_count >= m_capacity) {
    std::cerr << "AgentManager: capacity exceeded!" << std::endl;
    return -1;
  }

  cudaMemcpy(m_agents + m_count, &agent, sizeof(Agent), cudaMemcpyHostToDevice);
  return m_count++;
}
void AgentManager::Clear() {
  m_count = 0;
  cudaMemset(m_agents, 0, m_capacity * sizeof(Agent));
}


bool AgentManager::RemoveAgentAt(int idx, int* outSwappedFrom, int* outSwappedTo) {
  if (idx < 0 || (size_t)idx >= m_count)
    return false;
  const int last = (int)m_count - 1;
  if (outSwappedFrom)
    *outSwappedFrom = -1;
  if (outSwappedTo)
    *outSwappedTo = -1;

  if (idx != last) {
    // копируем последнего агента на место удаляемого
    cudaMemcpy(m_agents + idx, m_agents + last, sizeof(Agent), cudaMemcpyDeviceToDevice);
    if (outSwappedFrom)
      *outSwappedFrom = last;
    if (outSwappedTo)
      *outSwappedTo = idx;
  }
  // просто уменьшаем "логическое" количество
  --m_count;
  // (опционально) cudaGetLastError проверка
  return true;
}
