#pragma once
#include "ui/ITool.h"
#include "ui/ToolType.h"
#include <string_view> // +++

namespace ui {

class PlaceOnGroundTool final : public ITool {
public:
  // +++ добавь этот конструктор (можно ничего с параметром не делать)
  explicit PlaceOnGroundTool(std::string_view /*primaryModelPath*/) noexcept {}

  // ITool
  [[nodiscard]] ToolType     Type() const noexcept override;
  [[nodiscard]] const char*  Name() const noexcept ;
  [[nodiscard]] const char*  Id()   const noexcept ;

  void OnActivate(EditorContext& /*ctx*/) override {}
  void OnDeactivate(EditorContext& /*ctx*/) override {}

  void OnUpdate(EditorContext& ctx) override;
};

} // namespace ui
