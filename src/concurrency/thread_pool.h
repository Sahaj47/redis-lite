#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

class ThreadPool {
private:
    std::vector<std::thread> workers;
    
    // A queue that holds generic functions (our tasks)
    std::queue<std::function<void()>> tasks;
    
    // The lock to safely add/remove tasks from the queue
    std::mutex queue_mutex;
    
    // The "Alarm Clock" that wakes up sleeping threads when a task arrives
    std::condition_variable condition;
    
    // A flag to safely shut down the server when we are done
    bool stop;

public:
    // The constructor takes the number of threads we want to spawn
    ThreadPool(size_t threads);
    
    // The destructor cleans up all the threads
    ~ThreadPool();
    
    // This allows us to push a new function (like handling a client) into the queue
    void enqueue(std::function<void()> task);
};