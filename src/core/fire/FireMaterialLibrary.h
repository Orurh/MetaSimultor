#pragma once

#include "FireConfig.h"
#include "core/agent.h"  // FireCollider

#include <string>
#include <string_view>
#include <utility>

class FireMaterialLibrary {
 public:
  FireMaterialLibrary() = default;
  explicit FireMaterialLibrary(FireConfig cfg)
      : m_config(std::move(cfg)) {}

  void SetConfig(const FireConfig& cfg) { m_config = cfg; }
  [[nodiscard]] const FireConfig& GetConfig() const { return m_config; }

  // Вернуть параметры материала по строковому id ("wood", "grass", ...)
  [[nodiscard]] const FireMaterialParams& Get(std::string_view id) const;

  // Применить материал к коллайдеру: заполнить fuel/temperature/moisture/cond/ignition
  void ApplyToCollider(FireCollider& f_collider, std::string_view material_id) const;

 private:
  FireConfig m_config;
};