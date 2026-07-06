cmake_minimum_required(VERSION 3.20)

project(ThreadPool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Main executable

add_executable(server
    src/main.cpp
    src/threadPool.cpp
)

target_include_directories(server PRIVATE include)

target_compile_options(server PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -g
    -O1
    -fsanitize=thread
)

target_link_options(server PRIVATE
    -fsanitize=thread
)

target_link_libraries(server PRIVATE pthread)

# Tests

enable_testing()

set(TESTS
    basic_test
    future_test
    future_void_test
    exception_test
    stress_test
    shutdown_test
    work_stealing_test
)

foreach(test ${TESTS})

    add_executable(${test}
        tests/${test}.cpp
        src/threadPool.cpp
    )

    target_include_directories(${test} PRIVATE include)

    target_compile_options(${test} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -g
        -O1
        -fsanitize=thread
    )

    target_link_options(${test} PRIVATE
        -fsanitize=thread
    )

    target_link_libraries(${test} PRIVATE pthread)

    add_test(NAME ${test} COMMAND ${test})

endforeach()


# ---------------------------------------------------------
# Google Benchmark
# ---------------------------------------------------------

include(FetchContent)

FetchContent_Declare(
    benchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.8.1.zip
)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(benchmark)

# ---------------------------------------------------------
# Benchmarks
# ---------------------------------------------------------

set(BENCHMARKS
    enqueue_bench
    submit_bench
    cpu_bench
)

foreach(bench ${BENCHMARKS})

    add_executable(${bench}
        bench/${bench}.cpp
        src/threadPool.cpp
    )

    target_include_directories(${bench} PRIVATE include)

    target_compile_options(${bench} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -O3
    )

    target_link_libraries(${bench} PRIVATE
        benchmark::benchmark
        pthread
    )

endforeach()

# ---------------------------------------------------------
# std::async Benchmark
# (Doesn't use the thread pool implementation)
# ---------------------------------------------------------

add_executable(async_bench
    bench/async_bench.cpp
)

target_compile_options(async_bench PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -O3
)

target_link_libraries(async_bench PRIVATE
    benchmark::benchmark
    pthread
)
