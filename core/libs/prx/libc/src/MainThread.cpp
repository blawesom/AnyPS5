#include "prx/libc/include/MainThread.hpp"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <utility>

namespace {

struct MainThreadQueue {
    std::mutex mutex;
    std::condition_variable changed;
    std::deque<std::pair<void (*)(void*), void*>> tasks;
    std::atomic<bool> serving{false};
};

MainThreadQueue& Queue() {
    static auto* queue = new MainThreadQueue();
    return *queue;
}

}

extern "C" bool LibcMainThreadServing_nid_postfix() {
    return Queue().serving.load(std::memory_order_acquire);
}

extern "C" void LibcPostToMainThread_nid_postfix(void (*task)(void*), void* context) {
    auto& queue = Queue();
    {
        std::lock_guard lock(queue.mutex);
        queue.tasks.emplace_back(task, context);
    }
    queue.changed.notify_one();
}

extern "C" [[noreturn]] void LibcServeMainThread_nid_postfix() {
    auto& queue = Queue();
    queue.serving.store(true, std::memory_order_release);
    for (;;) {
        std::pair<void (*)(void*), void*> task;
        {
            std::unique_lock lock(queue.mutex);
            queue.changed.wait(lock, [&] { return !queue.tasks.empty(); });
            task = queue.tasks.front();
            queue.tasks.pop_front();
        }
        task.first(task.second);
    }
}
