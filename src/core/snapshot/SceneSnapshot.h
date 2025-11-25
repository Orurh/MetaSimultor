#pragma once
#include "CompactAgent.h"
#include <vector>
namespace engine::snapshot {
struct SceneState {
  std::vector<Agent> agents;
  glm::vec3 wind;
  float ambientTemp;
};
class SceneSnapshot {
  public:
  SceneSnapshot() = default;
  SceneSnapshot(const std::vector<Agent> &agents, const glm::vec3 &wind, float ambientTemp, double timestamp);
  SceneState Apply() const;
  auto GetTimestamp() const { return m_timestamp; }
  size_t GetMemoryUsage() const { return sizeof(*this) + m_agents.size() * sizeof(CompactAgent); }

  private:
  std::vector<CompactAgent> m_agents;
  glm::vec3 m_wind;
  float m_ambientTemp;
  double m_timestamp;
};
}  // namespace engine::snapshot