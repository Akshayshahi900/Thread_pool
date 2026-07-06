#include "threadPool.hpp"

#include <atomic>
#include <benchmark/benchmark.h>
#include <thread>

static void BM_EnqueueThroughput(benchmark::State &state) {
  constexpr int tasks = 100000;

  ThreadPool pool(state.range(0));

  for (auto _ : state) {
    std::atomic<int> counter{0};

    for (int i = 0; i < tasks; i++) {
      pool.enqueue([&] { counter.fetch_add(1, std::memory_order_relaxed); });
    }

    while (counter.load(std::memory_order_relaxed) != tasks)
      std::this_thread::yield();
  }

  state.counters["tasks/sec"] = benchmark::Counter(state.iterations() * tasks,
                                                   benchmark::Counter::kIsRate);
}

BENCHMARK(BM_EnqueueThroughput)->Arg(1)->Arg(2)->Arg(4)->Arg(8);

BENCHMARK_MAIN();
