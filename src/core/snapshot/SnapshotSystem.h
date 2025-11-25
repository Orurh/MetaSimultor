#pragma once
#include "SceneSnapshot.h"
#include <atomic>
#include <expected>
#include <thread>
namespace engine::snapshot {
class SnapshotSystem {
  public:
  SnapshotSystem() = default;
  void StartRecording();
  void StopRecording();

  bool IsRecording() const { return m_capturing; }
  void Update(double timestamp, const std::vector<Agent> &agents, const glm::vec3 &wind, float ambientTemt);

  std::expected<SceneSnapshot, std::string> GetSnapshot(double timestamp) const;
  std::expected<SceneState, std::string> GetSceneState(double timestamp) const;
  double GetStartTime() const;

  double GetEndTime() const;

  size_t GetSnapshotCount() const;

  size_t GetMemoryUsage() const;

  std::vector<double> GetAvailableTimestamps() const;

  private:
  std::vector<SceneSnapshot> m_snapshots;

  std::thread m_worker_thread;
  std::mutex m_snapshot_mutex;
  double m_last_snapshot_time = 0.0;
  double m_snapshot_interval = 1.0;
  std::atomic<bool> m_capturing{false};
};

}  // namespace engine::snapshot