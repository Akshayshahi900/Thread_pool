#include <exception>
#include <future>
#include <iostream>
#include <stdexcept>

int main() {
  // Step 1 Wrap a function
  std::packaged_task<int()> task([] {
    std::cout << "Task is executing...\n";
    throw std::runtime_error("BOOM");
  });

  // Step 2 Obtain the future
  std::future<int> future = task.get_future();

  std::cout << "Task has not executed yet.\n";

  // Step 3 Execute the task
  task();

  std::cout << "Task finished.\n";

  // Step 4 Read the result
  try {
    int answer = future.get();
    std::cout << answer << '\n';
  } catch (const std::exception &e) {
    std::cout << e.what() << '\n';
  }
  return 0;
}
