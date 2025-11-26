#pragma once

#include <chrono>
#include <ctime>

class SimulationTime {
public:
  using Clock = std::chrono::system_clock;

  SimulationTime() = default;

  // вызвать в момент инициализации сцены / приложения
  void InitToNow() {
    m_startWallClock = Clock::now();
    m_realTimeSinceStart = 0.0;
    m_simTime = 0.0;
    m_lastRealDt = 0.0;
    m_lastSimDt = 0.0;
  }

  // realDt — реальное dt из приложения (в секундах)
  void Update(double realDt) {
    // подстраховка от огромных dt при лагах / паузах
    const double MAX_REAL_DT = 0.1; // 100 мс
    double clamped = (realDt < 0.0) ? 0.0 : (realDt > MAX_REAL_DT ? MAX_REAL_DT : realDt);

    m_lastRealDt = clamped;
    m_realTimeSinceStart += clamped;

    if (m_paused) {
      m_lastSimDt = 0.0;
      return;
    }

    m_lastSimDt = clamped * m_timeScale;
    m_simTime += m_lastSimDt;
  }

  // === доступ к значениям ===
  [[nodiscard]] double GetRealTimeSinceStart() const { return m_realTimeSinceStart; } // сек
  [[nodiscard]] double GetSimTime() const { return m_simTime; }                       // сек
  [[nodiscard]] double GetLastRealDt() const { return m_lastRealDt; }
  [[nodiscard]] double GetLastSimDt() const { return m_lastSimDt; }

  [[nodiscard]] double GetTimeScale() const { return m_timeScale; }
  void SetTimeScale(double scale) {
    if (scale < 0.0) scale = 0.0;
    m_timeScale = scale;
  }

  bool IsPaused() const { return m_paused; }
  void SetPaused(bool paused) { m_paused = paused; }

  void TogglePause() { m_paused = !m_paused; }

  void ResetSimTime() {
    m_simTime = 0.0;
    m_lastSimDt = 0.0;
  }

  // === "реальное" время по локальным часам ===
  // локальное время запуска
  std::tm GetLocalStartTime() const {
    std::time_t t = Clock::to_time_t(m_startWallClock);
    std::tm result{};
#if defined(_WIN32)
    localtime_s(&result, &t);
#else
    result = *std::localtime(&t);
#endif
    return result;
  }

  // локальное "текущее" время (старт + realTimeSinceStart)
  std::tm GetLocalCurrentTime() const {
    auto now = m_startWallClock +
               std::chrono::duration_cast<Clock::duration>(
                   std::chrono::duration<double>(m_realTimeSinceStart));
    std::time_t t = Clock::to_time_t(now);
    std::tm result{};
#if defined(_WIN32)
    localtime_s(&result, &t);
#else
    result = *std::localtime(&t);
#endif
    return result;
  }

private:
  Clock::time_point m_startWallClock{Clock::now()};

  double m_realTimeSinceStart{0.0}; // сек с момента старта
  double m_simTime{0.0};            // "внутреннее" время симуляции
  double m_lastRealDt{0.0};
  double m_lastSimDt{0.0};

  double m_timeScale{0.3}; // 1.0 = realtime, 10.0 = в 10 раз быстрее
  bool   m_paused{false};
};
