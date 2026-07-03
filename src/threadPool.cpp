#include "threadPool.hpp"
#include "worker.hpp"
#include <iostream>
#include <random>
ThreadPool::ThreadPool(size_t n) {
  // initializing the vector of workers
  for (size_t i = 0; i < n; i++) {
    workers.emplace_back(std::make_unique<Worker>());
  }
  // calling the worker function for each worker
  for (size_t i = 0; i < n; i++) {
    workers[i]->thread_ = std::thread(&ThreadPool::worker, this, i);
  }
}
ThreadPool::~ThreadPool() {

  stop_.store(true, std::memory_order_release);
  for (auto &worker : workers) {
    worker->cv_.notify_all();
  }
  for (auto &worker : workers) {
    if (worker->thread_.joinable()) {
      worker->thread_.join();
    }
  }
};
void ThreadPool::enqueue(std::function<void()> task) {
  Job job;
  job.task = std::move(task);

  // choose worker
  size_t id =
      next_worker_.fetch_add(1, std::memory_order_relaxed) % workers.size();
  Worker &currentWorker = *workers[id];

  currentWorker.deque_.push_bottom(std::move(job));
  currentWorker.cv_.notify_one();
};

void ThreadPool::worker(size_t id) {

  Worker &current = *workers[id];
  while (true) {
    Job job;

    if (pop_local(id, job)) {
      try {
        job.task();
      } catch (const std::exception &e) {
        std::cerr << "Task Exception in worker " << id << ": " << e.what()
                  << "\n";
      } catch (...) {
        std::cerr << "Unknown exception in worker " << id << "\n";
      }
      continue;
    }
    if (stealJob(id, job)) {
      try {
        job.task();
      } catch (const std::exception &e) {
        std::cerr << "Task exception in worker " << id << ": " << e.what()
                  << "\n";
      } catch (...) {
        std::cerr << "Unknown exception in worker " << id << "\n";
      }
      continue;
    }

    std::unique_lock<std::mutex> lock(current.sleep_mutex);
    current.cv_.wait(lock, [&] { return stop_ || !current.deque_.empty(); });

    if (stop_ && current.deque_.empty()) {
      return;
    }
  }
}

bool ThreadPool::pop_local(size_t id, Job &job) {

  auto result = workers[id]->deque_.pop_bottom();
  if (!result) {
    return false;
  }
  job = std::move(*result);

  return true;
}

bool ThreadPool::stealJob(size_t id, Job &job) {
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
