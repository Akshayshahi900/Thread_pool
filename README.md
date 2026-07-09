# Work-Stealing Thread Pool

A high-performance fixed-size thread pool implemented in **Modern C++20** featuring **per-worker task deques**, **work stealing**, and **future-based asynchronous task execution**.

Unlike traditional thread pools that rely on a single global queue, this implementation assigns each worker its own local deque and employs a randomized work-stealing scheduler to balance load dynamically. This approach reduces queue contention, improves cache locality, and keeps idle workers productive by allowing them to steal pending tasks from busy workers.

The project was built to explore practical concurrent systems programming using modern C++ primitives, with emphasis on scheduler design, synchronization, scalability, and performance.

---

## Key Features

- Fixed-size worker thread pool
- Per-worker task deque architecture
- Randomized work-stealing scheduler
- Round-robin task placement
- `submit()` API returning `std::future<T>`
- Lightweight `enqueue()` API for fire-and-forget tasks
- Graceful shutdown with complete task draining
- Exception propagation through futures
- Modern C++20 implementation
- CMake-based build system
- CTest unit tests
- Google Benchmark performance suite

---

# Performance Highlights

Benchmarks were collected in **Release mode** using **Google Benchmark** on the development machine.

## Enqueue Throughput

| Worker Threads | Throughput |
|---------------:|-----------:|
| 1 | **2.93M tasks/s** |
| 2 | **4.23M tasks/s** |
| 4 | **2.10M tasks/s** |
| 8 | **1.61M tasks/s** |

## Submit Throughput

| Worker Threads | Throughput |
|---------------:|-----------:|
| 1 | **1.37M tasks/s** |
| 2 | **1.29M tasks/s** |
| 4 | **980K tasks/s** |
| 8 | **933K tasks/s** |

## Comparison with `std::async`

| Method | Throughput |
|---------|-----------:|
| `std::async` | **77K tasks/s** |
| Thread Pool (`submit`) | **933K tasks/s** |

The thread pool achieves approximately **12× higher submission throughput** than `std::async` by reusing a fixed set of worker threads instead of creating a new asynchronous execution context for every task.

---

# Why Work Stealing?

Many thread pools use a **single shared queue** for all tasks.

While simple, a global queue quickly becomes a synchronization bottleneck as more worker threads compete for the same lock. Increasing thread count eventually results in diminishing returns because workers spend more time waiting for the queue than executing tasks.

This implementation avoids that bottleneck by giving every worker its own local deque.

```
             Global Queue

      Worker 0 ─┐
      Worker 1 ─┼──► [ Shared Queue ]
      Worker 2 ─┤
      Worker 3 ─┘

        High lock contention
```

Each worker primarily interacts with its own queue.

When a worker becomes idle, it steals work from another worker instead of waiting for new tasks to arrive.

```
          Worker 0        Worker 1        Worker 2

        ┌──────────┐    ┌──────────┐    ┌──────────┐
        │ Deque 0  │    │ Deque 1  │    │ Deque 2  │
        └──────────┘    └──────────┘    └──────────┘
              ▲                              │
              └──────── Work Stealing ───────┘
```

This architecture provides several advantages:

- Reduced synchronization overhead
- Better CPU cache locality
- Improved load balancing
- Higher throughput under uneven workloads
- Better scalability as worker count increases

---

# Architecture Overview

The scheduling pipeline is intentionally simple to minimize overhead.

```
                  submit(task)
                       │
                       ▼
          Atomic Round-Robin Scheduler
                       │
     ┌─────────────────┼─────────────────┐
     ▼                 ▼                 ▼
 Worker 0          Worker 1          Worker N
 ┌──────────┐     ┌──────────┐     ┌──────────┐
 │  Deque   │     │  Deque   │     │  Deque   │
 └──────────┘     └──────────┘     └──────────┘
      ▲                                  │
      └────────── Work Stealing ─────────┘
```

Task submission consists of only three steps:

1. Select the next worker using an atomic round-robin counter.
2. Push the task into the selected worker's deque.
3. Notify the corresponding worker.

Workers always prioritize executing their own tasks. Only when their local deque becomes empty do they attempt to steal work from other workers.

# Internal Architecture

The thread pool consists of three primary components:

```
                   ThreadPool
                        │
        ┌───────────────┼────────────────┐
        │               │                │
        ▼               ▼                ▼
  Worker 0         Worker 1         Worker N
        │               │                │
        ▼               ▼                ▼
  Local Deque     Local Deque      Local Deque
        │               │                │
        ▼               ▼                ▼
 Worker Thread   Worker Thread    Worker Thread
```

## ThreadPool

The `ThreadPool` class is responsible for

- creating worker threads
- task submission
- work distribution
- work stealing
- worker shutdown
- synchronization during destruction

It owns the collection of workers and exposes the public interface used by clients.

---

## Worker

Each worker contains its own execution context and synchronization primitives.

```
Worker

├── std::thread
├── std::deque<Job>
├── std::mutex
├── std::condition_variable
└── stop flag
```

Each worker executes tasks independently from its local deque, minimizing interaction with other workers during normal execution.

---

## Job

Internally, every submitted task is represented as

```cpp
std::function<void()>
```

Tasks submitted through `submit()` are wrapped inside a `std::packaged_task` before being stored, enabling asynchronous result retrieval through `std::future`.

This abstraction allows the scheduler to execute all tasks uniformly regardless of their return type.

---

# Scheduling Strategy

Task placement and execution follow two simple principles:

1. Keep work local whenever possible.
2. Steal work only when necessary.

---

## Task Submission

Incoming tasks are distributed using an atomic round-robin counter.

```
Task 1 → Worker 0
Task 2 → Worker 1
Task 3 → Worker 2
Task 4 → Worker 3
Task 5 → Worker 0
...
```

Round-robin scheduling provides an even initial distribution while keeping scheduling overhead constant.

Unlike centralized scheduling algorithms, no worker is favored during task placement.

---

## Local Execution

Workers always execute tasks from the **back** of their own deque.

```
push_back()
pop_back()
```

This naturally creates **LIFO** execution.

Executing recently submitted work first often improves cache locality because the associated data is still resident in CPU caches.

The owner thread never removes tasks from the front of its own deque.

---

## Work Stealing

When a worker becomes idle, it begins searching for work from other workers.

The stealing procedure is straightforward:

1. Randomly choose another worker.
2. Attempt to lock its deque.
3. If tasks exist, steal one from the front.
4. Execute the stolen task immediately.
5. If unsuccessful, continue searching.

```
Victim Queue

Front ------------------------ Back
 ^                              ^
 |                              |
Stealing                  Local Execution
```

The owner thread and stealing thread typically operate on opposite ends of the deque, reducing contention compared to competing for the same queue position.

Random victim selection also helps distribute stealing attempts across workers rather than repeatedly targeting the same queue.

---

# Worker Lifecycle

Each worker repeatedly performs the following loop:

```
               Start Worker
                    │
                    ▼
          Is local queue empty?
             │             │
            No            Yes
             │             │
             ▼             ▼
      Execute task    Attempt stealing
             │             │
             └──────┬──────┘
                    ▼
           Any work available?
             │             │
            Yes           No
             │             │
             ▼             ▼
      Execute task     Wait on
                     condition_variable
                    │
                    ▼
             New task arrives
                    │
                    ▼
                 Wake up
```

Workers only sleep when

- their own deque is empty,
- stealing fails,
- and no additional work is available.

This avoids unnecessary busy waiting while still allowing quick response to newly submitted tasks.

---

# Synchronization

The implementation relies exclusively on standard C++20 synchronization primitives.

| Primitive | Purpose |
|-----------|---------|
| `std::mutex` | Protect worker deques |
| `std::condition_variable` | Suspend idle workers |
| `std::atomic` | Round-robin scheduling and shared state |
| `std::future` | Result retrieval |
| `std::packaged_task` | Wrap callable tasks |

Each worker owns its own mutex, avoiding a single global lock that would serialize task execution.

---

# Design Decisions

## Per-worker queues instead of a global queue

A global queue is straightforward to implement but becomes a synchronization hotspot under contention.

Assigning each worker its own deque significantly reduces lock contention and allows workers to execute most tasks without interacting with other threads.

---

## Round-robin task placement

Round-robin scheduling provides a predictable and evenly distributed initial workload while requiring only a single atomic increment.

More sophisticated scheduling strategies are possible, but their additional overhead is often unnecessary for general-purpose task execution.

---

## Front/Back deque ownership

The owner thread executes tasks from the back of its deque, while thieves steal from the front.

```
Front ------------------------- Back
  ▲                               ▲
  │                               │
Thief                         Owner
```

This separation minimizes contention because the owner and thief rarely compete for the same end of the deque.

---

## Randomized victim selection

Idle workers randomly choose a victim when attempting to steal work.

Random selection helps avoid repeated contention on a single worker and distributes stealing attempts more evenly across the thread pool.

---

## Futures for asynchronous results

The `submit()` interface returns a `std::future<T>` to provide standard C++ asynchronous semantics.

This allows callers to

- wait for completion,
- retrieve return values,
- and receive propagated exceptions,

without requiring custom synchronization mechanisms.

# Public API

The thread pool exposes two primary interfaces for task execution.

---

## `submit()`

Submits a callable object for asynchronous execution and returns a `std::future<T>` representing the eventual result.

```cpp
#include "threadPool.hpp"
#include <iostream>

int main() {
    ThreadPool pool(8);

    auto future = pool.submit([] {
        return 21 * 2;
    });

    std::cout << future.get() << '\n';
}
```

### Suitable for

- Tasks that produce a return value
- Exception propagation
- Synchronization through futures
- General asynchronous computation

---

## `enqueue()`

Schedules a task for execution without creating a future.

```cpp
ThreadPool pool(8);

pool.enqueue([] {
    performLogging();
});
```

Since no `std::future` is created, `enqueue()` introduces less overhead than `submit()` and is ideal for fire-and-forget workloads.

### Suitable for

- Logging
- Metrics collection
- Notifications
- Background maintenance tasks

---

# Project Structure

```
.
├── bench/
│   ├── async_bench.cpp
│   ├── cpu_bench.cpp
│   ├── enqueue_bench.cpp
│   └── submit_bench.cpp
│
├── include/
│   ├── job.hpp
│   ├── threadPool.hpp
│   └── worker.hpp
│
├── src/
│   └── threadPool.cpp
│
├── tests/
│   ├── basic_test.cpp
│   ├── exception_test.cpp
│   ├── future_test.cpp
│   ├── future_void_test.cpp
│   ├── shutdown_test.cpp
│   ├── stress_test.cpp
│   └── work_stealing_test.cpp
│
├── CMakeLists.txt
└── README.md
```

---

# Building

## Clone the repository

```bash
git clone https://github.com/Akshayshahi900/Thread_pool
cd thread-pool
```

## Configure

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

## Build

```bash
cmake --build build -j
```

---

# Running Tests

The project includes a comprehensive test suite using **CTest**.

Execute all tests with

```bash
cd build
ctest --output-on-failure
```

The test suite verifies

- Basic task execution
- Future correctness
- Void-returning tasks
- Exception propagation
- Graceful shutdown
- Stress scenarios
- Work-stealing correctness

---

# Running Benchmarks

Google Benchmark is used to evaluate scheduler performance.

Run individual benchmarks:

```bash
./build/enqueue_bench
./build/submit_bench
./build/cpu_bench
./build/async_bench
```

---

# Benchmark Analysis

## Enqueue Performance

| Workers | Throughput |
|---------:|-----------:|
| 1 | 2.93M tasks/s |
| 2 | 4.23M tasks/s |
| 4 | 2.10M tasks/s |
| 8 | 1.61M tasks/s |

Task enqueue throughput increases initially because submission work is distributed across multiple workers.

As the number of workers continues to increase, synchronization costs and scheduling overhead begin to outweigh the benefits of additional parallelism, leading to diminishing returns.

---

## Submit Performance

| Workers | Throughput |
|---------:|-----------:|
| 1 | 1.37M tasks/s |
| 2 | 1.29M tasks/s |
| 4 | 980K tasks/s |
| 8 | 933K tasks/s |

Unlike `enqueue()`, `submit()` constructs a `std::packaged_task` and returns a `std::future`.

The additional allocations and synchronization required to support futures reduce throughput, making `submit()` naturally more expensive than fire-and-forget task submission.

---

## Comparison with `std::async`

| Method | Throughput |
|---------|-----------:|
| `std::async` | 77K tasks/s |
| Thread Pool (`submit`) | 933K tasks/s |

The thread pool achieves approximately **12× higher throughput** than `std::async`.

The primary reason is that worker threads are created once and reused for every submitted task. In contrast, `std::async` may create a new execution context for each invocation, introducing significantly higher scheduling and creation overhead.

---

# Testing Philosophy

The goal of the test suite is to verify both correctness and robustness under concurrent execution.

The tests cover:

- Correct execution of scheduled tasks
- Preservation of return values
- Exception propagation through futures
- Graceful pool shutdown
- High-volume stress workloads
- Work stealing under uneven task distribution

While functional correctness is the primary objective, the stress tests also help uncover synchronization issues that may only appear under heavy contention.

---

# Design Trade-offs

This implementation intentionally prioritizes simplicity and correctness over maximum theoretical performance.

Several advanced optimizations commonly found in production schedulers were deliberately omitted to keep the implementation readable and educational.

Examples include:

- Lock-free Chase-Lev deques
- NUMA-aware scheduling
- Thread affinity
- Task priorities
- Dynamic worker resizing

These would improve scalability for specialized workloads but would also substantially increase implementation complexity.

---

# Future Improvements

Possible extensions include:

- Lock-free work-stealing deque
- NUMA-aware scheduling
- Thread affinity support
- Task priorities
- Cooperative cancellation using `std::stop_token`
- Dynamic thread pool resizing
- Coroutine integration
- Performance instrumentation
- Cache-line padding to reduce false sharing
- Wait-free submission path

---

# What I Learned

Building this project provided practical experience with several aspects of concurrent systems programming, including:

- Thread lifecycle management
- Condition variables
- Mutex synchronization
- Atomic operations
- Work-stealing schedulers
- Task scheduling strategies
- Asynchronous programming with futures
- Benchmark-driven performance evaluation
- Testing concurrent software

It also reinforced the importance of balancing scalability, synchronization overhead, and implementation complexity when designing concurrent systems.

---

# References

- *C++ Concurrency in Action* — Anthony Williams
- MIT OCW 6.172 Performance Engineering of Software Systems
- C++20 Standard Library Documentation
- Effective Modern C++ - Scott Meyers

---

# License

This project is released under the MIT License.
