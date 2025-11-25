#include "SceneSnapshot.h"
#include "core/snapshot/CompactAgent.h"
#include <algorithm>
#include <vector>
namespace engine::snapshot {
SceneSnapshot::SceneSnapshot(const std::vector<Agent> &agents, const glm::vec3 &wind, float ambientTemp,
                             double timestamp)
    : m_wind(wind), m_ambientTemp(ambientTemp), m_timestamp(timestamp) {
  m_agents.reserve(agents.size());

  for (const auto &agent : agents) {
    m_agents.emplace_back(SnapshotUtils::Compress(agent));
  }
}

SceneState SceneSnapshot::Apply() const {
  SceneState state;
  state.wind = m_wind;
  state.ambientTemp = m_ambientTemp;

  state.agents.resize(m_agents.size());
  std::transform(m_agents.begin(), m_agents.end(), state.agents.begin(), SnapshotUtils::Decompress);

  return state;
}
}  // namespace engine::snapshot