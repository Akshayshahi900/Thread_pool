#include "threadPool.hpp"

#include <atomic>
#include <cassert>

int main() {
  constexpr int tasks = 1000000;

  std::atomic<int> counter{0};

  {
    ThreadPool pool(8);

    for (int i = 0; i < tasks; i++) {
      pool.enqueue([&] { counter++; });
    }
  }

  assert(counter == tasks);
}
