// ui/Interface.h
#pragma once
namespace ui {
struct Interface {
  virtual ~Interface() = default;
  Interface() = default;
  Interface(const Interface &) = delete;
  Interface &operator=(const Interface &) = delete;
  Interface(Interface &&) = delete;
  Interface &operator=(Interface &&) = delete;
};
} // namespace ui
