// core/FireConfig.cc
#include "FireConfig.h"

FireConfig::FireConfig() : m_default_material_id("wood") {
  m_materials["wood"] = FireMaterialParams{
      .m_ignition_temperature = 600.0F,
      .m_fuel_density         = 100.0F,
      .m_conductivity         = 0.08F,
      .m_base_moisture        = 0.25F,
  };

  m_materials["grass"] = FireMaterialParams{
      .m_ignition_temperature = 500.0F,
      .m_fuel_density         = 40.0F,
      .m_conductivity         = 0.05F,
      .m_base_moisture        = 0.15F,
  };

  m_materials["house"] = FireMaterialParams{
      .m_ignition_temperature = 700.0F,
      .m_fuel_density         = 150.0F,
      .m_conductivity         = 0.12F,
      .m_base_moisture        = 0.20F,
  };
}

FireMaterialParams& FireConfig::Material(const std::string& id) {
  auto it = m_materials.find(id);
  if (it != m_materials.end()) {
    return it->second;
  }
  auto def_it = m_materials.find(m_default_material_id);
  if (def_it != m_materials.end()) {
    return def_it->second;
  }
  // если вдруг и дефолта нет — создаём "пустой" дефолт
  return m_materials[m_default_material_id];
}

const FireMaterialParams& FireConfig::Material(const std::string& id) const {
  auto it = m_materials.find(id);
  if (it != m_materials.end()) {
    return it->second;
  }
  auto def_it = m_materials.find(m_default_material_id);
  if (def_it != m_materials.end()) {
    return def_it->second;
  }
  return m_materials.begin()->second;
}

void BuildSimulationParams(const FireConfig& cfg,
                           FireSimulationParams& out_params) {
  out_params.m_base_check_distance   = 15.0F;
  out_params.m_base_spread_distance  = 3.0F;
  out_params.m_local_spread_distance = 2.0F;

  out_params.m_cross_agent_spread_strength = 10.0F;
  out_params.m_local_spread_strength       = 20.0F;

  // ВЛАЖНОСТЬ: просто пробрасываем коэффициенты из конфига
  out_params.m_humidity_heat_dampen     = cfg.m_env.m_humidity_dampen_coeff;
  out_params.m_humidity_ignition_factor = cfg.m_env.m_humidity_ignition_mult;

  // Глобальная влажность воздуха 0..1
  out_params.m_air_humidity_factor = cfg.m_env.m_air_humidity;

  // Ветер — базово 1.0, реальный бонус ставит FireSimulationSystem::Step
  out_params.m_wind_heat_bonus = 1.0F;

  out_params.m_base_conductivity = 0.1F;
  out_params.m_max_heat_distance = 8.0F;
}

