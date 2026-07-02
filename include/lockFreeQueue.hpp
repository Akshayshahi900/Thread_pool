#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <optional>

template <typename T, size_t capacity = 1024> class LockFreeDeque {
private:
  std::array<T, capacity> buffer_;
  std::atomic<size_t> top_{0};
  std::atomic<size_t> bottom_{0};

public:
  bool empty();
  bool push_bottom(T task);
  std::optional<T> pop_bottom();
  std::optional<T> steal_top();
};

template <typename T, size_t capacity>
bool LockFreeDeque<T, capacity>::push_bottom(T task) {
  size_t b = bottom_.load(std::memory_order_relaxed);

  buffer_[b % capacity] = std::move(task);

  bottom_.store(b + 1, std::memory_order_release);

  return true;
}

template <typename T, size_t capacity>
std::optional<T> LockFreeDeque<T, capacity>::steal_top() {
  while (true) {
    size_t t = top_.load(std::memory_order_acquire);
    size_t b = bottom_.load(std::memory_order_acquire);

    if (t >= b) {
      return std::nullopt;
    }

    T task = buffer_[t % capacity];
    if (top_.compare_exchange_strong(t, t + 1, std::memory_order_acquire,
                                     std::memory_order_relaxed)) {
      return task;
    }
  }
}

template <typename T, size_t capacity>
std::optional<T> LockFreeDeque<T, capacity>::pop_bottom() {
  size_t b = bottom_.load(std::memory_order_relaxed);

  if (b == 0)
    return std::nullopt;
  b--;
  bottom_.store(b, std::memory_order_relaxed);

  size_t t = top_.load(std::memory_order_acquire);

  if (t > b) {
    bottom_.store(t, std::memory_order_relaxed);
    return std::nullopt;
  }

  T task = buffer_[b % capacity];
  if (t == b) {
    if (!top_.compare_exchange_strong(t, t + 1, std::memory_order_acq_rel,
                                      std::memory_order_relaxed)) {
      bottom_.store(t + 1, std::memory_order_relaxed);
      return std::nullopt;
    }
    bottom_.store(t + 1, std::memory_order_relaxed);
  }
  return task;
}

template <typename T, size_t capacity>
bool LockFreeDeque<T, capacity>::empty() {
  return top_.load(std::memory_order_acquire) >=
         bottom_.load(std::memory_order_acquire);
}
