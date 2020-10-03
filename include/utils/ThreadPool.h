#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

// A utility class to handle a thread pool
// This class will handle the creation and destruction of threads,
// adding work to each thread and synchronizing the entire operation
class ThreadPool {
    using task_t = std::function<void()>;

  public:
    ThreadPool(uint8_t numberOfThreads);
    ~ThreadPool() { Stop(); }

    // Start the thread pool
    void Start();

    // Stop the thread pool by waiting for all threads to join
    void Stop();

    // Add new task to handle by the thread pool
    void AddTask(const task_t& task);

  private:
    // Loop untill the thread pool is stoped, once there is a work to be done
    // dispatch a free thread to handle it
    void WorkerLoop();

    std::mutex _qLock;
    std::condition_variable _qCv;
    std::deque<task_t> _taskQ;
    std::vector<std::thread> _threads;
    std::atomic<bool> _running;
    uint8_t _numberOfThreads;
};
