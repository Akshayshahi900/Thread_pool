#include <benchmark/benchmark.h>

#include <cmath>
#include <future>
#include <vector>

static void heavy_work() {
  double x = 0;

  for (int i = 1; i <= 10000; i++)
    x += std::sqrt(i);

  benchmark::DoNotOptimize(x);
}

static void BM_StdAsync(benchmark::State &state) {
  constexpr int tasks = 10000;

  for (auto _ : state) {
    std::vector<std::future<void>> futures;
    futures.reserve(tasks);

    for (int i = 0; i < tasks; i++) {
      futures.emplace_back(
          std::async(std::launch::async, [] { heavy_work(); }));
    }

    for (auto &future : futures)
      future.get();
  }

  state.counters["tasks/sec"] = benchmark::Counter(state.iterations() * tasks,
                                                   benchmark::Counter::kIsRate);
}

BENCHMARK(BM_StdAsync);

BENCHMARK_MAIN();
