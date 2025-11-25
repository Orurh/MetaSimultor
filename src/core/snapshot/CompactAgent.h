#pragma once
#include "../agent.h"
struct CompactAgent {
  Float3 position = {0, 0, 0};
  uint8_t agent_type = 0;
  float intensity = 0.0f;
  int16_t object_id = -1;
  uint8_t point_count = 0;

  float conductivity = 0.0f;
  float ignition_temp = 0.0f;
  float max_temperature = 1300.0f;
  Float3 bounds_min = {0, 0, 0};
  Float3 bounds_max = {0, 0, 0};
  uint16_t temperatures[64] = {0};
  uint8_t intensities[64] = {0};
  uint8_t fuels[64] = {0};
  uint8_t moisture[64] = {0};
  Float3 heat_points[64] = {};

  static constexpr size_t SIZE = 413 + (64 * sizeof(Float3));
};

namespace SnapshotUtils {
CompactAgent Compress(const Agent &source);
Agent Decompress(const CompactAgent &source);
}  // namespace SnapshotUtils
