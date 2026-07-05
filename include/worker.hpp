#pragma once

#include "job.hpp"

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

class Worker {
public:
  std::thread thread_;
  std::deque<Job> deque_;
  std::mutex mutex_;
  std::condition_variable cv_;
};
