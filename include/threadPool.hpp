#pragma once

#include "worker.hpp"

#include <atomic>
#include <future>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

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

  template <typename F> auto submit(F &&func) {
    using ReturnType = std::invoke_result_t<F>;

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::forward<F>(func));

    auto future = task->get_future();

    enqueue([task] { (*task)(); });

    return future;
  }
};
