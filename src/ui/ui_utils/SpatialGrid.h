// ui/utils/SpatialGrid.h
#pragma once
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include <glm/geometric.hpp>  // glm::dot
#include <glm/vec2.hpp>

namespace ui::util {

struct SpatialGrid {
  float cell = 1.0f;  // шаг сетки, обычно >= minSpacing
  std::unordered_map<std::int64_t, std::vector<glm::vec2>> buckets;

  static constexpr std::int64_t Key(int gx, int gy) noexcept {
    return (static_cast<std::int64_t>(gx) << 32) ^ static_cast<std::int64_t>(gy & 0xFFFFFFFF);
  }

  static constexpr std::pair<int, int> ToCell(const glm::vec2& p, float cell) noexcept {
    return {static_cast<int>(std::floor(p.x / cell)), static_cast<int>(std::floor(p.y / cell))};
  }

  void SetCell(float c) noexcept { cell = (c > 0.f ? c : 1.f); }
  void Clear() noexcept { buckets.clear(); }
  void Reserve(std::size_t nCells) { buckets.reserve(nCells); }  // опционально

  // Возвращает true, если точка принята (нет ближних соседей ближе minSpacing)
  bool Accept(const glm::vec2& p, float minSpacing) {
    const auto [cx, cy] = ToCell(p, cell);
    const float r2 = minSpacing * minSpacing;

    for (int dy = -1; dy <= 1; ++dy) {
      for (int dx = -1; dx <= 1; ++dx) {
        const auto it = buckets.find(Key(cx + dx, cy + dy));
        if (it == buckets.end())
          continue;
        for (const glm::vec2& q : it->second) {
          const glm::vec2 d = p - q;
          if (glm::dot(d, d) < r2)
            return false;
        }
      }
    }
    buckets[Key(cx, cy)].emplace_back(p);
    return true;
  }
};

}  // namespace ui::util
