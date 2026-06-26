#include <iostream>
#include <thread>
#include <functional>   
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>


struct Job{
    std::function<void ()> task;
};
class Worker{
    std::thread thread_ ;
    std::queue<Job> global_queue;
    std::mutex mutex_;
    std::atomic<bool>stop_;
    void run();

};
class ThreadPool{
    std::vector<Worker> workers_;
    std::queue<Job> global_queue_;
    std::mutex global_mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stop_;

public:
ThreadPool(int n);
~ThreadPool();
void submit(Job job);
};




