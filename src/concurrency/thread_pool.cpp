#include "thread_pool.h"

// 1. The Constructor (Hiring the Waiters)
ThreadPool::ThreadPool(size_t num_threads) : stop(false) {
    for (size_t i = 0; i < num_threads; ++i) {
        // Spawn a thread and give it an infinite loop of work
        workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;

                {
                    // Lock the queue before checking for tasks
                    std::unique_lock<std::mutex> lock(this->queue_mutex);

                    // THE ALARM CLOCK: Go to sleep UNTIL the server is stopping OR a task arrives
                    this->condition.wait(lock, [this] {
                        return this->stop || !this->tasks.empty();
                    });

                    // If the server is shutting down and the queue is empty, kill the thread
                    if (this->stop && this->tasks.empty()) {
                        return; 
                    }

                    // Grab the next task in line
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                } // The lock automatically releases here so other threads can access the queue

                // Execute the task (Serving the client)
                task();
            }
        });
    }
}

// 2. The Enqueue Method
void ThreadPool::enqueue(std::function<void()> task) {
    {
        // Lock the queue, add the new task, and unlock
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.push(task);
    }
    
    // Hit the alarm clock! Wake up exactly ONE sleeping thread to handle this task
    condition.notify_one();
}

// 3. The Destructor (Closing the Restaurant)
ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    // Wake up ALL sleeping threads so they can see the 'stop' flag and exit gracefully
    condition.notify_all();
    
    for (std::thread &worker : workers) {
        worker.join();
    }
}