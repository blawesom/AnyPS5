#include "prx/libSceVideoOut/include/PresentWorker.hpp"
#include "prx/libc/include/MainThread.hpp"

#include <stdexcept>
#include <utility>

void PresentWorker::Start(std::function<void(std::stop_token)> body) {
    if (Joinable()) throw std::runtime_error("PresentWorker: already started");
    if (!LibcMainThreadServing_nid_postfix()) {
        thread = std::jthread(std::move(body));
        return;
    }
    auto run = std::make_shared<MainThreadRun>();
    run->body = std::move(body);
    auto started = run->started.get_future();
    mainFinished = run->finished.get_future();
    mainRun = run;
    LibcPostToMainThread_nid_postfix(&PresentWorker::RunOnMainThread, new std::shared_ptr<MainThreadRun>(std::move(run)));
    mainId = started.get();
}

void PresentWorker::RunOnMainThread(void* context) {
    const auto run = *static_cast<std::shared_ptr<MainThreadRun>*>(context);
    delete static_cast<std::shared_ptr<MainThreadRun>*>(context);
    run->started.set_value(std::this_thread::get_id());
    try {
        run->body(run->stop.get_token());
        run->finished.set_value();
    } catch (...) {
        run->finished.set_exception(std::current_exception());
    }
}

void PresentWorker::RequestStop() {
    if (mainRun) mainRun->stop.request_stop();
    else thread.request_stop();
}

void PresentWorker::Join() {
    if (mainRun) {
        if (std::this_thread::get_id() == mainId) throw std::runtime_error("PresentWorker: the main thread cannot wait for its own presentation loop");
        auto finished = std::move(mainFinished);
        mainRun.reset();
        finished.get();
        return;
    }
    thread.join();
}

bool PresentWorker::Joinable() const {
    return mainRun != nullptr || thread.joinable();
}

bool PresentWorker::OnMainThread() const {
    return mainRun != nullptr;
}

std::thread::id PresentWorker::Id() const {
    return mainRun ? mainId : thread.get_id();
}
