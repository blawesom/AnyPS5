#include "prx/libkernel/File/include/DescriptorPoll.hpp"
#include "prx/libkernel/File/include/File.hpp"
#include "prx/libkernel/File/include/RandomDevice.hpp"

#ifdef _WIN32
#include "prx/libkernel/File/include/DirectoryDescriptor.hpp"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#else
#include <cerrno>
#include <poll.h>
#include <sys/stat.h>
#endif

#include <chrono>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr int GuestEbadf = 9;
constexpr short VnodeReady = KernelSocketPoll::Readable | KernelSocketPoll::Writable;

[[noreturn]] void Unsupported(const char* caller, int descriptor, const char* kind) {
    throw std::runtime_error(std::string(caller) + " on descriptor " + std::to_string(descriptor) + " (" + kind + ") not implemented");
}

bool AlwaysReady(int descriptor, const char* caller) {
    if (File::IsRandomDevice(descriptor)) Unsupported(caller, descriptor, "random device");
#ifdef _WIN32
    if (File::DirectoryDescriptorPath(descriptor)) return true;
    switch (::GetFileType(reinterpret_cast<HANDLE>(::_get_osfhandle(descriptor)))) {
        case FILE_TYPE_DISK: return true;
        case FILE_TYPE_CHAR: Unsupported(caller, descriptor, "Windows console or character device");
        case FILE_TYPE_PIPE: Unsupported(caller, descriptor, "Windows pipe");
        default: Unsupported(caller, descriptor, "Windows handle of unknown type");
    }
#else
    struct stat status{};
    if (::fstat(descriptor, &status) != 0)
        throw std::runtime_error(std::string(caller) + ": fstat failed for descriptor " + std::to_string(descriptor) + ", errno=" + std::to_string(errno));
    return S_ISREG(status.st_mode) || S_ISDIR(status.st_mode);
#endif
}

}

namespace File {

int PollHostDescriptors(KernelSocketPoll::Entry* entries, int count, int timeoutMilliseconds, const char* caller) {
    for (int index = 0; index < count; ++index)
        if (!DescriptorIsOpen_nid_no_patch(entries[index].descriptor)) return -GuestEbadf;
    std::vector<bool> alwaysReady;
    for (int index = 0; index < count; ++index) alwaysReady.push_back(AlwaysReady(entries[index].descriptor, caller));
    int ready = 0;
#ifndef _WIN32
    std::vector<pollfd> native;
    std::vector<int> owners;
#endif
    for (int index = 0; index < count; ++index) {
        auto& entry = entries[index];
        entry.revents = 0;
        if (alwaysReady[static_cast<std::size_t>(index)]) {
            entry.revents = static_cast<short>(entry.events & VnodeReady);
            if (entry.revents != 0) ++ready;
            continue;
        }
#ifndef _WIN32
        pollfd request{};
        request.fd = entry.descriptor;
        if (entry.events & KernelSocketPoll::Readable) request.events |= POLLIN;
        if (entry.events & KernelSocketPoll::Urgent) request.events |= POLLPRI;
        if (entry.events & KernelSocketPoll::Writable) request.events |= POLLOUT;
        native.push_back(request);
        owners.push_back(index);
#endif
    }
#ifndef _WIN32
    if (!native.empty()) {
        const int result = ::poll(native.data(), static_cast<nfds_t>(native.size()), ready != 0 ? 0 : timeoutMilliseconds);
        if (result < 0 && errno == EINTR) return ready;
        if (result < 0) throw std::runtime_error(std::string(caller) + ": host poll failed, errno=" + std::to_string(errno));
        for (std::size_t index = 0; index < native.size(); ++index) {
            const short events = native[index].revents;
            if (events & POLLNVAL) return -GuestEbadf;
            short revents = 0;
            if (events & POLLIN) revents |= KernelSocketPoll::Readable;
            if (events & POLLPRI) revents |= KernelSocketPoll::Urgent;
            if (events & POLLOUT) revents |= KernelSocketPoll::Writable;
            if (events & POLLERR) revents |= KernelSocketPoll::Error;
            if (events & POLLHUP) revents |= KernelSocketPoll::HangUp;
            entries[owners[index]].revents = revents;
            if (revents != 0) ++ready;
        }
        return ready;
    }
#endif
    if (ready == 0 && timeoutMilliseconds > 0) std::this_thread::sleep_for(std::chrono::milliseconds(timeoutMilliseconds));
    return ready;
}

}
