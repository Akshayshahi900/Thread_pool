#pragma once
#include "worker.hpp"
#include <atomic>
#include <random>
class ThreadPool {
public:
  std::vector<std::unique_ptr<Worker>> workers;
  std::condition_variable cv_;
  std::atomic<bool> stop_{false};
  std::atomic<size_t> next_worker_{0};

  ThreadPool(size_t n) {
    // initializing the vector of workers
    for (size_t i = 0; i < n; i++) {
      workers.emplace_back(std::make_unique<Worker>());
    }
    // calling the worker function for each worker
    for (int i = 0; i < n; i++) {
      workers[i]->thread_ = std::thread(&ThreadPool::worker, this, i);
    }
  }

  bool pop_local(size_t id, Job &job) {

    auto result = workers[id]->deque_.pop_bottom();
    if (!result) {
      return false;
    }
    job = std::move(*result);

    return true;
  }

  bool stealJob(size_t id, Job &job) {
    thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, workers.size() - 1);

    size_t start = dist(rng);

    for (size_t k = 0; k < workers.size(); k++) {
      size_t victim = (start + k) % workers.size();

      if (victim == id) {
        continue;
      }
      auto result = workers[victim]->deque_.steal_top();

      if (result) {
        job = std::move(*result);
        return true;
      }
    }
    return false;
  }

  void worker(size_t id) {

    Worker &current = *workers[id];
    while (true) {
      Job job;

      if (pop_local(id, job)) {
        job.task();
        continue;
      }
      if (stealJob(id, job)) {
        job.task();
        continue;
      }

      std::unique_lock<std::mutex> lock(current.sleep_mutex);
      current.cv_.wait(lock, [&] { return stop_ || !current.deque_.empty(); });

      if (stop_ && current.deque_.empty()) {
        return;
      }
    }
  }

  ~ThreadPool() {
    stop_ = true;
    for (auto &worker : workers) {
      worker->cv_.notify_all();
    }
    for (auto &worker : workers) {
      if (worker->thread_.joinable()) {
        worker->thread_.join();
      }
    }
  };
  void enqueue(std::function<void()> task) {
    Job job;
    job.task = std::move(task);

    // choose worker
    size_t id = next_worker_++ % workers.size();
    Worker &currentWorker = *workers[id];

    currentWorker.deque_.push_bottom(std::move(job));
    currentWorker.cv_.notify_one();
  };
};
