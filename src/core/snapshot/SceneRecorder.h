#pragma once
#include "SnapshotSystem.h"
#include "core/scene/Scene.h"
#include <stdexcept>
namespace engine {
class Scene;

class SceneRecorder {
  public:
  SceneRecorder(Scene *scene) : m_scene{scene} {
    if (!scene)
      throw std::invalid_argument("Scene cannot be null");
  }

  void Update(float dt);
  void StartRecording();
  void StopRecording();
  bool IsRecording() const;
  std::expected<snapshot::SceneState, std::string> GetSceneState(double timestamp) const {
    return m_snapshotSystem.GetSceneState(timestamp);
  }

  std::expected<snapshot::SceneSnapshot, std::string> GetSnapshot(double timestamp) const {
    return m_snapshotSystem.GetSnapshot(timestamp);
  }

  double GetRecordingStartTime() const { return m_snapshotSystem.GetStartTime(); }

  double GetRecordingEndTime() const { return m_snapshotSystem.GetEndTime(); }

  size_t GetSnapshotCount() const { return m_snapshotSystem.GetSnapshotCount(); }

  size_t GetMemoryUsage() const { return m_snapshotSystem.GetMemoryUsage(); }

  std::vector<double> GetAvailableTimestamps() const { return m_snapshotSystem.GetAvailableTimestamps(); }
  void ApplySceneState(const snapshot::SceneState &state);

  private:
  snapshot::SnapshotSystem m_snapshotSystem;

  Scene *m_scene;
};
}  // namespace engine