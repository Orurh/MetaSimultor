#pragma once

#include "FireConfig.h"
#include "core/agent.h"  // FireCollider

#include <string>
#include <string_view>
#include <utility>

class FireMaterialLibrary {
 public:
  FireMaterialLibrary() = default;

  explicit FireMaterialLibrary(FireConfig& cfg) noexcept
      : m_config(&cfg) {}

  void SetConfig(FireConfig& cfg) noexcept { m_config = &cfg; }

  [[nodiscard]] const FireConfig& GetConfig() const {
    assert(m_config && "FireMaterialLibrary::m_config is null");
    return *m_config;
  }

  [[nodiscard]] const FireMaterialParams& Get(std::string_view id) const;
  void ApplyToCollider(FireCollider& f_collider,
                       std::string_view material_id) const;

 private:
  FireConfig* m_config = nullptr;  // важно: инициализировать
};
