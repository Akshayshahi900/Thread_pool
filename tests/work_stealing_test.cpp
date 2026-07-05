#include "threadPool.hpp"

#include <cassert>
#include <chrono>
#include <mutex>
#include <set>
#include <thread>

int main() {

  std::mutex m;
  std::set<std::thread::id> ids;

  constexpr int tasks = 10000;

  {
    ThreadPool pool(8);

    for (int i = 0; i < tasks; i++) {
      pool.enqueue([&] {
        {
          std::lock_guard<std::mutex> lock(m);
          ids.insert(std::this_thread::get_id());
        }

        std::this_thread::sleep_for(std::chrono::microseconds(100));
      });
    }

  } // <- ThreadPool destructor waits for all workers here

  assert(ids.size() > 1);

  return 0;
}
