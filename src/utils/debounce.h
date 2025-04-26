#pragma once

#include <functional>

class Debounce {
  std::function<void()> callback;
  unsigned int interval = 0;
  unsigned int ms;

 public:
  Debounce(unsigned int ms, const std::function<void()>& callback);
  ~Debounce();
  void call();
};
