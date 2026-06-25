#include "threadpool.h"

Threadpool::ThreadPool(size_t numThreads){
    for(size_t i =0; i < numThreads ; i++){
        workers.emplace_back(&ThreadPool::workerLoop , this);
    }
}


