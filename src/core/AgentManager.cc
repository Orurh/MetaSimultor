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

Agent AgentManager::AddTree(int tree_id, float base_x, float base_z, float scale) {
  Agent tree;
  tree.type = NORMAL;
  tree.object_id = tree_id;
  tree.position = {base_x, 0.0f, base_z};

  setupTreeFireCollider(tree, base_x, base_z, scale);

  AddAgent(tree);
  return tree;
}

// void AgentManager::AddFire(float x, float y, float z, float intensity) {
//   Agent a;
//   a.position = Float3{x, y, z};
//   a.type = AgentType::BURNING;             // или NORMAL, если у тебя огонь
//   «разжигает» со временем a.intensity = intensity;                  // или
//   твоя логика начального значения

//   // если нужен коллайдер — инициализируй его локально:
//   a.fire_collider.point_count = 1;
//   a.fire_collider.heat_points[0] = Float3{0.f, 0.f, 0.f};
//   a.fire_collider.fuels[0] = 100.f;
//   a.fire_collider.intensities[0] = intensity;

//   AddAgent(a); // как ты обычно пушишь в буфер + на GPU
// }

void AgentManager::setupTreeFireCollider(Agent& tree, float base_x, float base_z, float scale) {
  const float BASE_WIDTH = 2.0f;
  const float BASE_HEIGHT = 8.0F;
  tree.fire_collider.bounds_min.x = base_x - BASE_WIDTH * scale;
  tree.fire_collider.bounds_min.y = 0.0f;
  tree.fire_collider.bounds_min.z = base_z - BASE_WIDTH * scale;

  tree.fire_collider.bounds_max.x = base_x + BASE_WIDTH * scale;
  tree.fire_collider.bounds_max.y = BASE_HEIGHT * scale;
  tree.fire_collider.bounds_max.z = base_z + BASE_WIDTH * scale;

  generateTreeHeatPoints(tree.fire_collider, base_x, base_z, scale);
}

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

void AgentManager::generateTreeHeatPoints(FireCollider& collider, float base_x, float base_z,
                                          float scale) {
  collider.point_count = 0;
  const float BASE_TRUNK_HEIGHT_STEP = 0.8F;
  const float BASE_MAX_HEIGHT = 8.0F;
  const float BASE_START_HEIGHT = 0.5F;

  for (float height = BASE_START_HEIGHT * scale; height <= BASE_START_HEIGHT * scale;
           height += BASE_TRUNK_HEIGHT_STEP * scale) {
    collider.heat_points[collider.point_count] = Float3(base_x, height, base_z);
    collider.fuels[collider.point_count] = 30.0f;
    collider.intensities[collider.point_count] = 0.0f;
    collider.point_count++;
  }

  // Базовые параметры уровней короны
  float base_levels[] = {2.0f, 4.0f, 6.0f, 7.5f};
  float base_level_radii[] = {1.0f, 1.8f, 2.2f, 1.5f};
  int points_per_level[] = {8, 12, 16, 8};

  for (int level = 0; level < 4; level++) {
    // Масштабируем высоту и радиус
    float height = base_levels[level] * scale;
    float radius = base_level_radii[level] * scale;
    int points = points_per_level[level];

    for (int i = 0; i < points; i++) {
      float angle = (float)i / points * 360.0f;
      float rad = glm::radians(angle);

      collider.heat_points[collider.point_count].x = base_x + cos(rad) * radius;
      collider.heat_points[collider.point_count].y = height;
      collider.heat_points[collider.point_count].z = base_z + sin(rad) * radius;
      collider.fuels[collider.point_count] = 20.0f;
      collider.intensities[collider.point_count] = 0.0f;
      collider.point_count++;

      if (collider.point_count >= 64)
        break;
    }
    if (collider.point_count >= 64)
      break;
  }

  // Случайные точки тоже масштабируем
  std::random_device rd;
  std::mt19937 gen(rd());

  // Масштабируем область генерации случайных точек
  float random_area = 2.0f * scale;
  std::uniform_real_distribution<float> dist_x(base_x - random_area, base_x + random_area);
  std::uniform_real_distribution<float> dist_z(base_z - random_area, base_z + random_area);
  std::uniform_real_distribution<float> dist_y(1.0f * scale, 7.0f * scale);

  while (collider.point_count < 50 && collider.point_count < 64) {
    collider.heat_points[collider.point_count].x = dist_x(gen);
    collider.heat_points[collider.point_count].y = dist_y(gen);
    collider.heat_points[collider.point_count].z = dist_z(gen);
    collider.fuels[collider.point_count] = 15.0f;
    collider.intensities[collider.point_count] = 0.0f;
    collider.point_count++;
  }
}

void AgentManager::AddFire(float x, float z, float intensity) {
  AddFireAt(Float3{x, 0.0f, z}, intensity);
}

void AgentManager::AddFireAt(const Float3& world, float intensity) {
  if (m_count >= m_capacity)
    return;

  Agent fire{};
  fire.type = BURNING;
  fire.object_id = -1;
  fire.position = world;

  fire.fire_collider.point_count = 1;
  fire.fire_collider.heat_points[0] = world;  // мировая точка источника
  fire.fire_collider.fuels[0] = 100.0f;
  fire.fire_collider.intensities[0] = intensity;

  fire.fire_collider.bounds_min = Float3(world.x - 0.25f, world.y - 0.25f, world.z - 0.25f);
  fire.fire_collider.bounds_max = Float3(world.x + 0.25f, world.y + 0.25f, world.z + 0.25f);

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
Agent AgentManager::CreateAgent(AgentType type, Float3 position,
                                const AgentComponents& components) {
  Agent agent;
  agent.type = type;
  agent.position = position;
  agent.object_id = components.object_id;
  agent.intensity = components.intencity;

  agent.fire_collider.bounds_min = components.collider_bounds_min;
  agent.fire_collider.bounds_max = components.collider_bounds_max;

  float scale = components.scale;
  generateTreeHeatPoints(agent.fire_collider, position.x, position.z, scale);
  AddAgent(agent);
  return agent;
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
