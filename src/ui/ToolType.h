#pragma once
#include <cstdint>

namespace ui {

// Единое определение перечисления + фиксированный базовый тип
enum class ToolType : std::uint8_t {
  PLACE_ON_GROUND = 0,
  OTHER_TOOL,
};

} // namespace ui
