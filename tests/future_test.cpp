#include "threadPool.hpp"
#include <cassert>

int main() {
  ThreadPool pool(4);

  auto future = pool.submit([] { return 18; });

  assert(future.get() == 18);
}
