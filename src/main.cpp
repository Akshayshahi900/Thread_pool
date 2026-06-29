#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

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
    for (int i = 0; i < n; i++) {
      workers[i]->thread_ = std::thread(&ThreadPool::worker, this, i);
    }
  }
  void worker(size_t id) {
    // grab current worker
    Worker &currentWorker = *workers[id];

    // run till the end of life of the threadpool
    while (true) {
      std::unique_lock<std::mutex> lock(currentWorker.mutex_);

      // sleep untill the pool is stopping or the deque has atleast one job
      currentWorker.cv_.wait(
          lock, [&] { return stop_ || !currentWorker.deque_.empty(); });

      // check for
      if (stop_ && currentWorker.deque_.empty()) {
        return;
      }
      Job job = std::move(currentWorker.deque_.back());
      currentWorker.deque_.pop_back();

      lock.unlock();

      job.task();
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
};

int main() {
  ThreadPool pool(10);
  for (int i = 1; i <= 10; i++) {
    pool.enqueue([i] {
      std::cout << "Task" << i << "executed by thread "
                << "id :" << std::this_thread::get_id() << "\n\n";
    });
  }
  return 0;
}
