#include "threadPool.hpp"

#include <benchmark/benchmark.h>
#include <future>
#include <vector>

static void BM_SubmitThroughput(benchmark::State &state) {
  constexpr int tasks = 100000;

  ThreadPool pool(state.range(0));

  for (auto _ : state) {
    std::vector<std::future<void>> futures;
    futures.reserve(tasks);

    for (int i = 0; i < tasks; i++)
      futures.emplace_back(pool.submit([] {}));

    for (auto &future : futures)
      future.get();
  }

  state.counters["tasks/sec"] = benchmark::Counter(state.iterations() * tasks,
                                                   benchmark::Counter::kIsRate);
}

BENCHMARK(BM_SubmitThroughput)->Arg(1)->Arg(2)->Arg(4)->Arg(8);

BENCHMARK_MAIN();
