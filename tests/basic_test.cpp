#include "threadPool.hpp"
#include <atomic>
#include <cassert>

int main() {
  constexpr int tasks = 10000;
  std::atomic<int> counter{0};
  {
    ThreadPool pool(4);
    for (int i = 0; i < tasks; i++) {
      pool.enqueue([&] { counter++; });
    }
  }
  assert(counter == tasks);
}
