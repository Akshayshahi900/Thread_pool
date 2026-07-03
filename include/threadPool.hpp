#pragma once
#include "job.hpp"
#include "worker.hpp"
#include <atomic>
class ThreadPool {
public:
  std::vector<std::unique_ptr<Worker>> workers;
  std::condition_variable cv_;
  std::atomic<bool> stop_{false};
  std::atomic<size_t> next_worker_{0};

public:
  ThreadPool(size_t n);
  ~ThreadPool();
  bool pop_local(size_t id, Job &job);
  bool stealJob(size_t id, Job &job);
  void worker(size_t id);
  void enqueue(std::function<void()> task);
};
