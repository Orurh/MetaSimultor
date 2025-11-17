// core/FireMaterialLibrary.cpp
#include "FireMaterialLibrary.h"

const FireMaterialParams& FireMaterialLibrary::Get(std::string_view id) const {
  // Используем FireConfig::Material, он уже делает fallback на дефолт
  // конвертируем string_view -> string (id маленькиe, это дёшево)
  return m_config->Material(std::string{id});
}

void FireMaterialLibrary::ApplyToCollider(FireCollider& f_collider,
                                          std::string_view material_id) const {
  const FireMaterialParams& m = Get(material_id);

  f_collider.conductivity  = m.m_conductivity;
  f_collider.ignition_temp = m.m_ignition_temperature;

  for (int i = 0; i < f_collider.point_count; ++i) {
    f_collider.fuels[i]        = m.m_fuel_density;
    f_collider.intensities[i]  = 0.0F;
    f_collider.temperatures[i] = m_config->m_env.m_ambient_temperature;
    f_collider.moisture[i]     = m.m_base_moisture;
  }
}

