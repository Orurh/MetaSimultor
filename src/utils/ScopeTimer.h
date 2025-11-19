// ScopeTimer.h
#pragma once
#include <chrono>
#include <iostream>
#include <string>

class ScopeTimer {
  public:
  ScopeTimer(const std::string &name) : m_name(name), m_start(std::chrono::high_resolution_clock::now()) {}

  ~ScopeTimer() {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start);
    double milliseconds = duration.count() / 1000.0;
    std::cout << "[" << m_name << "] " << std::fixed << std::setprecision(3) << milliseconds << " ms\n";
  }

  private:
  std::string m_name;
  std::chrono::high_resolution_clock::time_point m_start;
};