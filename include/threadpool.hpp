#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include<thread>
class Threadpool
{
public:
     Threadpool(size_t numThreads);
     ~Threadpool();

     void submit(std::function<void()> job);

private:
     void workerLoop();

private:
     std::vector<std::thread> workers;

     std::queue<std::function<void()>> jobs;

     std::mutex queueMutex;
     std::condition_variable cv;

     bool stop = false;

};