#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <type_traits>
#include <utility>

struct Job {
  std::function<void()> task;
};

class Worker {
public:
  std::thread thread_;
  std::deque<Job> deque_;
  std::mutex mutex_;
  std::condition_variable cv_;
};

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
    for (size_t i = 0; i < n; i++) {
      workers[i]->thread_ = std::thread(&ThreadPool::worker, this, i);
    }
  }

  bool pop_local(size_t id, Job &job) {
    Worker &worker = *workers[id];
    std::lock_guard<std::mutex> lock(worker.mutex_);

    if (worker.deque_.empty()) {
      return false;
    }

    job = std::move(worker.deque_.back());
    worker.deque_.pop_back();

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

  void worker(size_t id) {
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

    // scope it and take the lock and push the job into the queue
    {
      std::lock_guard<std::mutex> lock(currentWorker.mutex_);
      currentWorker.deque_.push_back(std::move(job));
    }
    currentWorker.cv_.notify_one();
  };
  template <typename F> auto submit(F &&func) {
    using ReturnType = std::invoke_result_t<F>;

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::forward<F>(func));

    auto future = task->get_future();

    enqueue([task] { (*task)(); });

    return future;
  }
};

int main() {

  ThreadPool pool(5);

  auto future = pool.submit([] { return 43; });

  std::cout << future.get() << '\n';
}
