// ui/panels/FireStatsPanel.h
#pragma once

#include "core/agent.h"  // Agent, FireCollider
#include "ui/EditorContext.h"
#include "ui/IPanel.h"

#include <array>

namespace ui {

class FireStatsPanel final : public IPanel {
 public:
  [[nodiscard]] const char* Id() const override { return "Fire Stats"; }
  void Draw(EditorContext& ctx) override;

 private:
  static constexpr int HISTORY = 512;

  // Кольцевой буфер
  std::array<float, HISTORY> m_time{};
  std::array<float, HISTORY> m_area{};
  std::array<float, HISTORY> m_radius{};
  std::array<float, HISTORY> m_speed{};
  std::array<float, HISTORY> m_moisture{};

  int m_head = 0;   // куда писать следующий элемент
  int m_count = 0;  // сколько реально заполнено (<= HISTORY)
  float m_simTime = 0.0F;
  bool m_paused = false;

  // Текущие метрики (для вывода цифрами)
  float m_lastArea = 0.0f;
  float m_lastRadius = 0.0f;
  float m_lastSpeed = 0.0f;
  float m_lastMoisture = 0.0f;

  void Reset();
  void Sample(EditorContext& ctx);
};

}  // namespace ui
