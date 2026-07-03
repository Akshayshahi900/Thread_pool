#include "threadPool.hpp"
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

int main() {
  ThreadPool pool(4);

  std::atomic<int> counter{0};

  constexpr int tasks = 1000;

  for (int i = 0; i < tasks; i++) {
    pool.enqueue(
        [&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
  }

  std::this_thread::sleep_for(std::chrono::seconds(1));

  std::cout << "Counter = " << counter << '\n';
  return 0;
}
