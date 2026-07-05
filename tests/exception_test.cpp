#include "threadPool.hpp"

#include <cassert>
#include <stdexcept>

int main() {
  ThreadPool pool(4);

  auto future = pool.submit([]() -> int {
    throw std::runtime_error("boom");
  });

  bool caught = false;

  try {
    future.get();
  } catch (...) {
    caught = true;
  }

  assert(caught);
}
