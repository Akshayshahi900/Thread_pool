#include "threadPool.hpp"
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

bool ThreadPool::pop_local(size_t id, Job &job) {
  Worker &worker = *workers[id];
  std::lock_guard<std::mutex> lock(worker.mutex_);

  if (worker.deque_.empty()) {
    return false;
  }

  job = std::move(worker.deque_.back());
  worker.deque_.pop_back();

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

    Worker &target = *workers[victim];

    std::unique_lock<std::mutex> lock(target.mutex_, std::try_to_lock);

    if (!lock.owns_lock())
      continue;

    if (!target.deque_.empty()) {
      job = std::move(target.deque_.front());

      target.deque_.pop_front();
      return true;
    }
  }
  return false;
}

void ThreadPool::worker(size_t id) {
  // grab current worker

  // run till the end of life of the threadpool
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

    Worker &current = *workers[id];
    std::unique_lock<std::mutex> lock(current.mutex_);
    current.cv_.wait(lock, [&] { return stop_ || !current.deque_.empty(); });

    if (stop_ && current.deque_.empty()) {
      return;
    }
  }
}

ThreadPool::~ThreadPool() {
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
void ThreadPool::enqueue(std::function<void()> task) {
  Job job;
  job.task = std::move(task);

  // choose worker
  size_t id = next_worker_++ % workers.size();
  Worker &currentWorker = *workers[id];

  // scope it and take the lock and push the job into the queue
  {
    std::lock_guard<std::mutex> lock(currentWorker.mutex_);
    currentWorker.deque_.push_back(std::move(job));
  }
  currentWorker.cv_.notify_one();
};
