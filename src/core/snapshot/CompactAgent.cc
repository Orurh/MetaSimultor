#include "CompactAgent.h"
#include <cstdint>

namespace SnapshotUtils {
CompactAgent Compress(const Agent &source) {
  CompactAgent compressed;
  compressed.position = source.position;
  compressed.agent_type = static_cast<uint8_t>(source.type);
  compressed.intensity = source.intensity;
  compressed.object_id = source.object_id;
  compressed.point_count = source.fire_collider.point_count;

  compressed.conductivity = source.fire_collider.conductivity;
  compressed.ignition_temp = source.fire_collider.ignition_temp;
  compressed.max_temperature = source.fire_collider.max_temperature;
  compressed.bounds_min = source.fire_collider.bounds_min;
  compressed.bounds_max = source.fire_collider.bounds_max;
  for (int i = 0; i < compressed.point_count && i < 64; ++i) {
    compressed.temperatures[i] = static_cast<uint16_t>(source.fire_collider.temperatures[i] * 10.0f);
    compressed.intensities[i] = static_cast<uint8_t>(source.fire_collider.intensities[i] * 255.0f);
    compressed.fuels[i] = static_cast<uint8_t>(source.fire_collider.fuels[i] * 2.55f);
    compressed.moisture[i] = static_cast<uint8_t>(source.fire_collider.moisture[i] * 2.55f);
    compressed.heat_points[i] = source.fire_collider.heat_points[i];
  }

  return compressed;
}
Agent Decompress(const CompactAgent &source) {
  Agent agent;

  agent.position = source.position;
  agent.type = static_cast<FireAgentState>(source.agent_type);
  agent.intensity = source.intensity;
  agent.object_id = source.object_id;
  agent.fire_collider.max_temperature = source.max_temperature;
  agent.fire_collider.bounds_min = source.bounds_min;
  agent.fire_collider.bounds_max = source.bounds_max;
  agent.fire_collider.conductivity = source.conductivity;
  agent.fire_collider.ignition_temp = source.ignition_temp;

  agent.fire_collider.point_count = source.point_count;
  for (int i = 0; i < source.point_count && i < 64; ++i) {
    agent.fire_collider.temperatures[i] = source.temperatures[i] / 10.0f;
    agent.fire_collider.intensities[i] = source.intensities[i] / 255.0f;
    agent.fire_collider.fuels[i] = source.fuels[i] / 2.55f;
    agent.fire_collider.moisture[i] = source.moisture[i] / 2.55f;
    agent.fire_collider.heat_points[i] = source.heat_points[i];
  }

  for (int i = source.point_count; i < 64; ++i) {
    agent.fire_collider.temperatures[i] = 0.0f;
    agent.fire_collider.intensities[i] = 0.0f;
    agent.fire_collider.fuels[i] = 0.0f;
    agent.fire_collider.moisture[i] = 0.0f;
  }

  return agent;
}
}  // namespace SnapshotUtils
