#include "SnapshotSystem.h"
#include "core/snapshot/SceneSnapshot.h"
#include <algorithm>
#include <expected>
#include <string>

namespace engine::snapshot {
std::expected<SceneSnapshot, std::string> SnapshotSystem::GetSnapshot(double timestamp) const {
  auto it = std::lower_bound(m_snapshots.begin(), m_snapshots.end(), timestamp,
                             [](const SceneSnapshot &snapshot, double ts) { return snapshot.GetTimestamp() < ts; });
  if (it == m_snapshots.end() || std::abs(it->GetTimestamp() - timestamp) > 0.4) {
    return std::unexpected("No snapshots found at frame " + std::to_string(timestamp));
  }
  return *it;
}

std::expected<SceneState, std::string> SnapshotSystem::GetSceneState(double timestamp) const {
  auto snapshot = GetSnapshot(timestamp);
  if (!snapshot) {
    return std::unexpected(snapshot.error());
  }
  return snapshot->Apply();
}
double SnapshotSystem::GetStartTime() const {
  if (m_snapshots.empty())
    return 0.0;
  return m_snapshots.front().GetTimestamp();
}

double SnapshotSystem::GetEndTime() const {
  if (m_snapshots.empty())
    return 0.0;
  return m_snapshots.back().GetTimestamp();
}

size_t SnapshotSystem::GetSnapshotCount() const { return m_snapshots.size(); }

size_t SnapshotSystem::GetMemoryUsage() const {
  size_t total = 0;
  for (const auto &snapshot : m_snapshots) {
    total += snapshot.GetMemoryUsage();
  }
  return total;
}

std::vector<double> SnapshotSystem::GetAvailableTimestamps() const {
  std::vector<double> timestamps;
  timestamps.reserve(m_snapshots.size());
  for (const auto &snapshot : m_snapshots) {
    timestamps.push_back(snapshot.GetTimestamp());
  }
  return timestamps;
}

void SnapshotSystem::StartRecording() {
  m_capturing = true;
  m_snapshots.clear();
  m_last_snapshot_time = 0.0;
}

void SnapshotSystem::StopRecording() { m_capturing = false; }

void SnapshotSystem::Update(double timestamp, const std::vector<Agent> &agents, const glm::vec3 &wind,
                            float ambientTemp) {
  if (!m_capturing)
    return;

  if (timestamp - m_last_snapshot_time >= m_snapshot_interval) {
    m_snapshots.emplace_back(agents, wind, ambientTemp, timestamp);
    m_last_snapshot_time = timestamp;
  }
}
}  // namespace engine::snapshot