#include "threadPool.hpp"
#include <cassert>

int main() {
  ThreadPool pool(4);
  bool done = false;

  auto future = pool.submit([&] { done = true; });
  future.get();
  assert(done);
}
