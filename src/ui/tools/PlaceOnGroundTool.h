#pragma once
#include "../ITool.h"
#include "ui/ToolType.h"
#include <string_view> 

struct GLFWwindow;
namespace engine { class Terrain; }

namespace ui {

class PlaceOnGroundTool final : public ITool {
public:
  explicit PlaceOnGroundTool(std::string_view /*primaryModelPath*/) noexcept {}

  // ITool (не меняем)
  [[nodiscard]] ToolType    Type() const noexcept override;
  [[nodiscard]] const char* Name() const noexcept override;
  [[nodiscard]] const char* Id()   const noexcept override;

  void OnActivate(EditorContext& /*ctx*/)   override {}
  void OnDeactivate(EditorContext& /*ctx*/) override {}
  void OnUpdate(EditorContext& ctx) override;

  // Переключатель: false — плоскость Y=0 (как сейчас), true — точный raycast по террейну
  void UseTerrainRaycast(bool v) noexcept { m_useTerrain = v; }

private:

  // 1) курсор окна (логические координаты окна)
  [[nodiscard]] static glm::dvec2 ReadMouseWindow(GLFWwindow* win) ;

  // 2) окно → панель (логические пиксели панели); false если курсор вне панели
  [[nodiscard]] static bool WindowToViewportLogical(const EditorContext& ctx,
                                             const glm::dvec2& mouseWin,
                                             glm::vec2& outVpLogical);

  // 3) логические пиксели панели → framebuffer-пиксели (учёт HiDPI)
  [[nodiscard]] static glm::vec2 ToFramebufferPx(const EditorContext& ctx,
                                          const glm::vec2& vpLogical);

  // 4) построить луч из пикселя фреймбуфера
  static void BuildScreenRay(const EditorContext& ctx, const glm::vec2& px,
                      glm::vec3& outOrigin, glm::vec3& outDir);

  // 5а) пересечение с плоскостью Y=0
  [[nodiscard]] static bool RaycastPlaneY0(const glm::vec3& ro, const glm::vec3& rd,
                                    glm::vec3& outHit);

  // 5б) пересечение с террейном (шаговый поиск + бинарка), anchorY = -bounds_min.y*scale.y
  [[nodiscard]] static bool RaycastTerrain(const engine::Terrain& T,
                                    const glm::vec3& ro, const glm::vec3& rd,
                                    float anchorY,
                                    glm::vec3& outHit);


     // Возвращает true, если удаление обработано (и дальнейшая логика не нужна)
  bool HandleDeleteClick(EditorContext& ctx, const glm::vec3& ro, const glm::vec3& rd);

  // Жест удаления: ПКМ или Ctrl+ЛКМ
  static bool IsDeleteGesture() noexcept;

  // Радиус подбора по XZ при удалении (можете подвинтить)
  static constexpr float K_PICK_RADIUS_XZ = 3.0F;                           

  bool m_useTerrain{false};
};

} // namespace ui
