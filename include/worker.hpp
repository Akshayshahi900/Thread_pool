#pragma once
#include "job.hpp"
#include "lockFreeQueue.hpp"
#include <condition_variable>
#include <mutex>
#include <thread>
class Worker {
public:
  std::thread thread_;
  LockFreeDeque<Job> deque_;
  std::mutex sleep_mutex;
  std::condition_variable cv_;
  std::mutex push_mutex_;
};
