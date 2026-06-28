#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

struct Job {
  std::function<void()> task;
};
class Worker {
  std::thread thread_;
  std::deque<Job> deque;
  std::mutex mutex_;
  std::atomic<bool> stop_;
};
class ThreadPool {
public:
  ThreadPool(size_t thread_count) {
    for (size_t i = 0; i < thread_count; i++) {
      workers_.emplace_back(&ThreadPool::worker, this);
    }
  }

  ~ThreadPool() {
    stop_ = true;
    cv_.notify_all();

    for (auto &thread : workers_) {
      if (thread.joinable())
        thread.join();
    }
  };

  void enqueue(std::function<void()> task) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      tasks_.push(std::move(task));
    }
    cv_.notify_one();
  }

private:
  void worker() {
    while (true) {
      std::function<void()> task;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
        if (stop_ && tasks_.empty())
          return;
        task = std::move(tasks_.front());
        tasks_.pop();
      }
      task();
    }
  }

private:
  std::vector<std::thread> workers_;
  // std::queue<std::function<void()>> tasks_;
  // std::mutex mutex_;
  // std::condition_variable cv_;
  std::atomic<bool> stop_{false};
};

int main() {
  ThreadPool pool(10);
  for (int i = 1; i <= 10; i++) {
    pool.enqueue([i] {
      std::cout << "Task" << i << " executed by thread "
                << std::this_thread::get_id() << '\n';
    });
  }

  return 0;
}
