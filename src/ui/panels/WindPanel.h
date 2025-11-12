#pragma once
#include "ui/IPanel.h"

namespace ui {

class WindPanel final : public IPanel {
public:
    [[nodiscard]] const char* Id() const override { return "Wind Configuration"; }
    void Draw(EditorContext& ctx) override;

private:
    float m_direction[3] = {1.0f, 0.0f, 0.0f};
    float m_strength = 10.0f;
    float m_gustiness = 0.3f;
    float m_changeSpeed = 0.1f;
    
    // Для визуализации
    bool m_showWindVisualization = true;
};

} // namespace ui