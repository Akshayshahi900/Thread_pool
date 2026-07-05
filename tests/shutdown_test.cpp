#include "threadPool.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <thread>

int main() {
  constexpr int tasks = 1000;

  std::atomic<int> counter{0};

  {
    ThreadPool pool(4);

    for (int i = 0; i < tasks; i++) {
      pool.enqueue([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        counter++;
      });
    }
  }

  assert(counter == tasks);
}
