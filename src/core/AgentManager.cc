#include "AgentManager.h"
#include <cuda_runtime.h>
#include <iostream>
#include <random>

AgentManager::AgentManager(int max_agents) : m_capacity(max_agents), m_count(0) {
  cudaMalloc(&m_agents, max_agents * sizeof(Agent));
}

AgentManager::~AgentManager() {
  cudaFree(m_agents);
}

Agent AgentManager::AddTree(int tree_id, float base_x, float base_z) {
  Agent tree;
  tree.type = NORMAL;
  tree.object_id = tree_id;
  tree.position = {base_x, 0.0f, base_z};    // мир
  setupTreeFireCollider(tree);               // ЛОКАЛЬНЫЙ коллайдер
  AddAgent(tree);
  return tree;
}

// void AgentManager::AddFire(float x, float y, float z, float intensity) {
//   Agent a;
//   a.position = Float3{x, y, z};
//   a.type = AgentType::BURNING;             // или NORMAL, если у тебя огонь «разжигает» со временем
//   a.intensity = intensity;                  // или твоя логика начального значения

//   // если нужен коллайдер — инициализируй его локально:
//   a.fire_collider.point_count = 1;
//   a.fire_collider.heat_points[0] = Float3{0.f, 0.f, 0.f};
//   a.fire_collider.fuels[0] = 100.f;
//   a.fire_collider.intensities[0] = intensity;

//   AddAgent(a); // как ты обычно пушишь в буфер + на GPU
// }


void AgentManager::IgniteRandomPoint(int agent_index) {
  if (agent_index < 0 || agent_index >= m_count)
    return;
  Agent agent;
  cudaMemcpy(&agent, m_agents + agent_index, sizeof(Agent), cudaMemcpyDeviceToHost);

  if (agent.fire_collider.point_count > 0) {
    int random_point = rand() % agent.fire_collider.point_count;
    agent.fire_collider.intensities[random_point] = 1.0f;
    cudaMemcpy(m_agents + agent_index, &agent, sizeof(Agent), cudaMemcpyHostToDevice);
  }
}

void AgentManager::setupTreeFireCollider(Agent& tree) {
  // ЛОКАЛЬНЫЕ границы относительно origin агента
  tree.fire_collider.bounds_min = Float3(-2.0f, 0.0f, -2.0f);
  tree.fire_collider.bounds_max = Float3(+2.0f, 8.0f, +2.0f);
  generateTreeHeatPoints(tree.fire_collider);
}

void AgentManager::generateTreeHeatPoints(FireCollider& c) {
  c.point_count = 0;

  // ствол по центру
  for (float h = 0.5f; h <= 8.0f && c.point_count < 64; h += 0.8f) {
    c.heat_points[c.point_count] = Float3(0.f, h, 0.f);
    c.fuels[c.point_count] = 30.f;
    c.intensities[c.point_count] = 0.f;
    ++c.point_count;
  }

  const float levels[]       = {2.0f, 4.0f, 6.0f, 7.5f};
  const float level_radii[]  = {1.0f, 1.8f, 2.2f, 1.5f};
  const int   pts_per_level[] = {8, 12, 16, 8};

  for (int L = 0; L < 4 && c.point_count < 64; ++L) {
    const float h = levels[L], r = level_radii[L];
    const int   N = pts_per_level[L];
    for (int i = 0; i < N && c.point_count < 64; ++i) {
      const float a = (float)i / N * 6.28318530718f;
      c.heat_points[c.point_count] = Float3(std::cos(a)*r, h, std::sin(a)*r);
      c.fuels[c.point_count] = 20.f;
      c.intensities[c.point_count] = 0.f;
      ++c.point_count;
    }
  }

  // опционально — ещё немного случайных локальных точек внутри AABB
  if (c.point_count < 64) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dx(c.bounds_min.x, c.bounds_max.x);
    std::uniform_real_distribution<float> dy(c.bounds_min.y, c.bounds_max.y);
    std::uniform_real_distribution<float> dz(c.bounds_min.z, c.bounds_max.z);

    while (c.point_count < 50 && c.point_count < 64) {
      c.heat_points[c.point_count] = Float3(dx(gen), dy(gen), dz(gen));
      c.fuels[c.point_count] = 15.0f;
      c.intensities[c.point_count] = 0.0f;
      ++c.point_count;
    }
  }
}


void AgentManager::AddFire(float x, float z, float intensity) {
  AddFireAt(Float3{x, 0.0f, z}, intensity);
}

void AgentManager::AddFireAt(const Float3& world, float intensity) {
  if (m_count >= m_capacity) return;

  Agent fire{};
  fire.type = BURNING;
  fire.object_id = -1;
  fire.position = world; // мировое положение источника

  fire.fire_collider.point_count = 1;
  fire.fire_collider.heat_points[0] = Float3(0.f, 0.f, 0.f); // ЛОКАЛЬНО!
  fire.fire_collider.fuels[0] = 100.f;
  fire.fire_collider.intensities[0] = intensity;

  fire.fire_collider.bounds_min = Float3(-0.25f, -0.25f, -0.25f);
  fire.fire_collider.bounds_max = Float3(+0.25f, +0.25f, +0.25f);

  AddAgent(fire);
}


void AgentManager::setupFireSourceCollider(Agent& fire, float x, float y) {
  fire.fire_collider.bounds_min.x = x - 0.5f;
  fire.fire_collider.bounds_min.y = 0.0f;
  fire.fire_collider.bounds_min.z = y - 0.5f;

  fire.fire_collider.bounds_max.x = x + 0.5f;
  fire.fire_collider.bounds_max.y = 2.0f;
  fire.fire_collider.bounds_max.z = y + 0.5f;

  fire.fire_collider.point_count = 1;
  fire.fire_collider.heat_points[0].x = x;
  fire.fire_collider.heat_points[0].y = 1.0f;
  fire.fire_collider.heat_points[0].z = y;
  fire.fire_collider.fuels[0] = 50.0f;
  fire.fire_collider.intensities[0] = 0.0f;
}

void AgentManager::AddAgent(const Agent& agent) {
  if (m_count >= m_capacity) {
    std::cerr << "AgentManager: capacity exceeded!" << std::endl;
    return;
  }

  cudaMemcpy(m_agents + m_count, &agent, sizeof(Agent), cudaMemcpyHostToDevice);
  m_count++;
}
void AgentManager::Clear() {
  m_count = 0;
  cudaMemset(m_agents, 0, m_capacity * sizeof(Agent));
}
Agent AgentManager::CreateAgent(AgentType type, Float3 position,
                                const AgentComponents& c) {
  Agent a;
  a.type = type;
  a.position = position;
  a.object_id = c.object_id;
  a.intensity = c.intencity;

  a.fire_collider.bounds_min = c.collider_bounds_min; // локально
  a.fire_collider.bounds_max = c.collider_bounds_max;
  // НЕ генерим здесь точки из головы
  // generateTreeHeatPoints(a.fire_collider, ...); // удалить

  AddAgent(a);
  return a;
}


bool AgentManager::RemoveAgentAt(int idx, int* outFrom, int* outTo) {
  if (idx < 0 || (size_t)idx >= m_count)
    return false;
  const int last = (int)m_count - 1;
  if (outFrom)
    *outFrom = -1;
  if (outTo)
    *outTo = -1;

  if (idx != last) {
    // копируем последнего агента на место удаляемого
    cudaMemcpy(m_agents + idx, m_agents + last, sizeof(Agent), cudaMemcpyDeviceToDevice);
    if (outFrom)
      *outFrom = last;
    if (outTo)
      *outTo = idx;
  }
  // просто уменьшаем "логическое" количество
  --m_count;
  // (опционально) cudaGetLastError проверка
  return true;
}



Agent AgentManager::GetAgentCPU(int idx) const {
  Agent a{};
  if (idx < 0 || static_cast<size_t>(idx) >= m_count) {
    std::cerr << "GetAgentCPU: idx out of range: " << idx << "\n";
    return a;
  }
  cudaError_t err = cudaMemcpy(&a, m_agents + idx, sizeof(Agent), cudaMemcpyDeviceToHost);
  if (err != cudaSuccess) {
    std::cerr << "GetAgentCPU: cudaMemcpy D2H failed: " << cudaGetErrorString(err) << "\n";
  }
  return a;
}

void AgentManager::UpdateAgentCPU(int idx, const Agent& a) {
  if (idx < 0 || static_cast<size_t>(idx) >= m_count) {
    std::cerr << "UpdateAgentCPU: idx out of range: " << idx << "\n";
    return;
  }
  cudaError_t err = cudaMemcpy(m_agents + idx, &a, sizeof(Agent), cudaMemcpyHostToDevice);
  if (err != cudaSuccess) {
    std::cerr << "UpdateAgentCPU: cudaMemcpy H2D failed: " << cudaGetErrorString(err) << "\n";
  }
}

std::vector<Agent> AgentManager::DownloadAllAgentsCPU() const {
  std::vector<Agent> v(m_count);
  if (m_count == 0) return v;
  cudaError_t err = cudaMemcpy(v.data(), m_agents, m_count * sizeof(Agent), cudaMemcpyDeviceToHost);
  if (err != cudaSuccess) {
    std::cerr << "DownloadAllAgentsCPU: cudaMemcpy D2H failed: " << cudaGetErrorString(err) << "\n";
    v.clear();
  }
  return v;
}

void AgentManager::UploadAllAgentsCPU(const std::vector<Agent>& v) {
  const size_t n = v.size();
  if (n > m_capacity) {
    std::cerr << "UploadAllAgentsCPU: too many agents (" << n << " > " << m_capacity << ")\n";
    return;
  }
  if (n == 0) {
    m_count = 0;
    return;
  }
  cudaError_t err = cudaMemcpy(m_agents, v.data(), n * sizeof(Agent), cudaMemcpyHostToDevice);
  if (err != cudaSuccess) {
    std::cerr << "UploadAllAgentsCPU: cudaMemcpy H2D failed: " << cudaGetErrorString(err) << "\n";
    return;
  }
  m_count = n;
}
