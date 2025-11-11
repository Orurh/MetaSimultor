// ui/tools/FoliageBrushTool.h
#pragma once
#include "ui/ITool.h"
#include <string>
#include <random>
#include <vector>
#include <glm/vec3.hpp>

namespace engine { class Scene; }
namespace ui {
class EditorContext;

class FoliageBrushTool final : public ITool {
public:
  enum class Shape { Circle, Rect };

  struct Settings {
    Shape shape = Shape::Circle;
    float radius = 10.f;        // для круга
    float width  = 20.f;        // для прямоугольника
    float height = 20.f;        // для прямоугольника
    float density = 0.1f;       // объектов на м² (например)
    float minSpacing = 1.0f;    // минимальная дистанция между точками
    float jitter = 0.5f;        // доля случайного смещения в ячейке
    unsigned seed = 12345;

    // визуальное разнообразие
    float scaleMin = 0.9f;
    float scaleMax = 1.3f;
    float yawMinDeg = 0.f;
    float yawMaxDeg = 360.f;

    // какой префаб/тип дерева
    std::string prefab = "default_tree";
  };

  explicit FoliageBrushTool(std::string_view /*unused*/) {}

  [[nodiscard]] ToolType Type() const noexcept override { return ToolType::OTHER_TOOL; }
  [[nodiscard]] const char* Name() const noexcept { return "Foliage Brush"; }
  [[nodiscard]] const char* Id()   const noexcept { return "Tool.FoliageBrush"; }

  void OnActivate(EditorContext& ctx) override;
  void OnDeactivate(EditorContext& /*ctx*/) override { m_preview.clear(); m_dragging = false; }
  void OnUpdate(EditorContext& ctx) override;

  // панель параметров (вызови из Panels/Toolbar или отдельной панели)
  void DrawUI(EditorContext& ctx);

private:
  // выбор области
  bool m_dragging = false;
  glm::vec3 m_anchor{};     // первая точка (мир)
  glm::vec3 m_current{};    // текущая точка (мир)
  std::vector<glm::vec3> m_preview; // точки предпросмотра (мир)

  Settings m_settings{};
  std::mt19937 m_rng{m_settings.seed};

  // генераторы
  void RebuildPreviewCircle();
  void RebuildPreviewRect();
  void ApplyPlacement(engine::Scene& scene);

  // хелперы
  static bool ScreenToGround(EditorContext& ctx, float mx, float my, glm::vec3& outHit);
  void EnsureSeed();


  bool m_hasCenter = false;
    glm::vec3 m_center{0.0f};  // центр области по клику

    void DrawOverlay(EditorContext& ctx);           // рисование контура и точек
    static bool ProjectToScreen(const EditorContext& ctx,
                                const glm::vec3& p_world,
                                glm::vec2& out_px); // world -> screen px

};

} // namespace ui
