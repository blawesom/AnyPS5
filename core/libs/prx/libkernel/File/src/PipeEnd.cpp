#include "prx/libkernel/File/include/PipeEnd.hpp"

#include <map>
#include <mutex>

namespace {

std::mutex g_mutex;
std::map<int, File::PipeEnd> g_ends;

}

namespace File {

void RememberPipeEnd(int fd, PipeEnd end) {
    std::lock_guard lock(g_mutex);
    g_ends[fd] = end;
}

std::optional<PipeEnd> FindPipeEnd(int fd) {
    std::lock_guard lock(g_mutex);
    const auto found = g_ends.find(fd);
    if (found == g_ends.end()) return std::nullopt;
    return found->second;
}

void ForgetPipeEnd(int fd) {
    std::lock_guard lock(g_mutex);
    g_ends.erase(fd);
}

}
