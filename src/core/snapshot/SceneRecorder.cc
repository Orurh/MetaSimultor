#include "SceneRecorder.h"
#include "core/scene/Scene.h"
namespace engine {
void SceneRecorder::Update(float dt) {
  if (m_scene && m_snapshotSystem.IsRecording()) {
    auto agents = m_scene->LoadAgentsFromGPU();
    auto wind = m_scene->GetWindConfig().getCurrentWind(m_scene->GetWindTime());
    float ambientTemp = m_scene->GetFireModule().Config().m_env.m_ambient_temperature;

    m_snapshotSystem.Update(m_scene->GetSimTimeSeconds(), agents, wind, ambientTemp);
  }
}

void SceneRecorder::StartRecording() { m_snapshotSystem.StartRecording(); }
void SceneRecorder::StopRecording() { m_snapshotSystem.StopRecording(); }
bool SceneRecorder::IsRecording() const { return m_snapshotSystem.IsRecording(); }
void SceneRecorder::ApplySceneState(const snapshot::SceneState &state) {
  if (!m_scene)
    return;
  m_scene->GetAgentManager().SetAgents(state.agents);
  m_scene->GetFireModule().Config().m_env.m_ambient_temperature = state.ambientTemp;
  m_scene->GetWindConfig().setWind(state.wind);

  auto agents = m_scene->LoadAgentsFromGPU();
  float ambientTemp = m_scene->GetFireModule().Config().m_env.m_ambient_temperature;
  m_scene->GetHeatMap().UpdateFromAgents(agents, ambientTemp);
}

}  // namespace engine