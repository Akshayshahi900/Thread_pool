#pragma once
#include <functional>
struct Job {
  std::function<void()> task;
};
