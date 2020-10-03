#include "utils/ThreadPool.h"

ThreadPool::ThreadPool(uint8_t numberOfThreads) : _running(false), _numberOfThreads(numberOfThreads) { Start(); }

void ThreadPool::Start() {
    if (!_running) {
        _running = true;

        for (uint32_t i = 0; i < _numberOfThreads; i++) {
            _threads.push_back(std::move(std::thread([this] { WorkerLoop(); })));
        }
    }
}

void ThreadPool::Stop() {
    _running = false;

    // wait for all threads to finish
    for (auto& t : _threads) {
        _qCv.notify_all();

        if (t.joinable()) {
            t.join();
        }
    }
}

void ThreadPool::AddTask(const task_t& task) {
    {
        std::unique_lock<std::mutex> l(_qLock);
        _taskQ.push_back(task);
    }

    _qCv.notify_all();
}

void ThreadPool::WorkerLoop() {
    while (_running) {
        task_t w = nullptr;

        {
            std::unique_lock<std::mutex> l(_qLock);

            _qCv.wait(l, [this] { return !_taskQ.empty() || !_running; });

            if (!_taskQ.empty()) {
                w = _taskQ.front();
                _taskQ.pop_front();
            }
        }

        if (w != nullptr) {
            w();
        }
    }
}
