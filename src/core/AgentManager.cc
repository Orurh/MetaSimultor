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

AgentManager::AgentManager(int max_agents) : m_capacity(max_agents), m_count(0), m_agents(nullptr) {
  if (m_capacity > 0) {
    cudaError_t err = cudaMalloc(&m_agents, m_capacity * sizeof(Agent));
    if (err != cudaSuccess) {
      std::cerr << "AgentManager: cudaMalloc failed: " << cudaGetErrorString(err) << '\n';
      m_agents = nullptr;
      m_capacity = 0;
    }
  }
}

AgentManager::~AgentManager() {
  cudaFree(m_agents);
}

std::vector<Agent> AgentManager::LoadAgentsFromGPU() {
  const size_t CURRENT_COUNT = GetAgentCount();
  std::vector<Agent> agents(CURRENT_COUNT);

  // если нечего читать или буфер не выделен — сразу выходим
  if (CURRENT_COUNT == 0 || m_agents == nullptr) {
    return agents;
  }

  cudaError_t err = cudaMemcpy(agents.data(), m_agents, CURRENT_COUNT * sizeof(Agent), cudaMemcpyDeviceToHost);

  if (err != cudaSuccess) {
    std::println("CUDA memcpy (D2H) error: {}", cudaGetErrorString(err));
  }

  return agents;
}

void AgentManager::AddFireAt(const Float3& world, float intensity) {
  if (m_count >= m_capacity || m_agents == nullptr) {
    std::cerr << "AgentManager::AddFireAt: no capacity or device buffer\n";

    return;
  }

  Agent fire{};
  fire.type = FireAgentState::Burning;
  fire.object_id = -1;
  fire.position = world;

  fire.fire_collider.point_count = 1;
  fire.fire_collider.heat_points[0] = world;  // мировая точка источника
  fire.fire_collider.fuels[0] = 200.0F;
  fire.fire_collider.intensities[0] = intensity;

  const float HALF_HEIGHT = FIRE_SOURCE_HEIGHT * 0.5F;

  fire.fire_collider.bounds_min = Float3(world.x - FIRE_SOURCE_BASE_RADIUS, world.y - HALF_HEIGHT, world.z - FIRE_SOURCE_BASE_RADIUS);

  fire.fire_collider.bounds_max = Float3(world.x + FIRE_SOURCE_BASE_RADIUS, world.y + HALF_HEIGHT, world.z + FIRE_SOURCE_BASE_RADIUS);

  AddAgent(fire);
}

int AgentManager::AddAgent(const Agent& agent) {
  if (m_count >= m_capacity || m_agents == nullptr) {
    std::cerr << "AgentManager::AddAgent: capacity exceeded or no device buffer!\n";
    return -1;
  }

  cudaError_t err = cudaMemcpy(m_agents + m_count, &agent, sizeof(Agent), cudaMemcpyHostToDevice);

  if (err != cudaSuccess) {
    std::cerr << "AgentManager::AddAgent: cudaMemcpy (H2D) failed: " << cudaGetErrorString(err) << '\n';
    return -1;
  }
  return m_count++;
}

void AgentManager::Clear() {
  m_count = 0;
  if ((m_agents != nullptr) && m_capacity > 0) {
    cudaError_t err = cudaMemset(m_agents, 0, m_capacity * sizeof(Agent));
    if (err != cudaSuccess) {
      std::cerr << "AgentManager::Clear: cudaMemset failed: " << cudaGetErrorString(err) << '\n';
    }
  }
}

bool AgentManager::RemoveAgentAt(int idx, int* outSwappedFrom, int* outSwappedTo) {
  if (idx < 0 || static_cast<size_t>(idx) >= m_count || m_agents == nullptr) {
    return false;
  }

  const int LAST = m_count - 1;
  if (outSwappedFrom != nullptr) {
    *outSwappedFrom = -1;
  }
  if (outSwappedTo != nullptr)
    *outSwappedTo = -1;

  if (idx != LAST) {
    cudaError_t err = cudaMemcpy(m_agents + idx, m_agents + LAST, sizeof(Agent), cudaMemcpyDeviceToDevice);

    if (err != cudaSuccess) {
      std::cerr << "AgentManager::RemoveAgentAt: cudaMemcpy (D2D) failed: " << cudaGetErrorString(err) << '\n';
      return false;
    }

    if (outSwappedFrom != nullptr) {
      *outSwappedFrom = LAST;
    }
    if (outSwappedTo != nullptr) {
      *outSwappedTo = idx;
    }
  }

  --m_count;
  return true;
}
