#ifndef CORE_LIBS_PRX_LIBSCEVIDEOOUT_INCLUDE_PRESENTWORKER_HPP
#define CORE_LIBS_PRX_LIBSCEVIDEOOUT_INCLUDE_PRESENTWORKER_HPP

#include <functional>
#include <future>
#include <memory>
#include <stop_token>
#include <thread>

class PresentWorker final {
public:
    void Start(std::function<void(std::stop_token)> body);
    void RequestStop();
    void Join();
    bool Joinable() const;
    bool OnMainThread() const;
    std::thread::id Id() const;

private:
    struct MainThreadRun {
        std::function<void(std::stop_token)> body;
        std::stop_source stop;
        std::promise<void> finished;
        std::promise<std::thread::id> started;
    };

    static void RunOnMainThread(void* context);

    std::jthread thread;
    std::shared_ptr<MainThreadRun> mainRun;
    std::future<void> mainFinished;
    std::thread::id mainId;
};

#endif
