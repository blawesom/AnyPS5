#if defined(__linux__) || defined(__APPLE__)

#include <cerrno>
#include <system_error>
#include <sys/resource.h>
#if defined(__APPLE__)
#include <climits>
#include <sys/sysctl.h>
#endif

namespace {

struct OpenFileLimit {
    OpenFileLimit() {
        rlimit limit{};
        if (getrlimit(RLIMIT_NOFILE, &limit) != 0) throw std::system_error(errno, std::generic_category(), "read open file limit");
        if (limit.rlim_cur >= limit.rlim_max) return;
#if defined(__APPLE__)
        int perProcess = 0;
        std::size_t size = sizeof(perProcess);
        if (sysctlbyname("kern.maxfilesperproc", &perProcess, &size, nullptr, 0) == 0 && perProcess > 0 && static_cast<rlim_t>(perProcess) < limit.rlim_max) limit.rlim_max = static_cast<rlim_t>(perProcess);
        limit.rlim_cur = limit.rlim_max;
        if (setrlimit(RLIMIT_NOFILE, &limit) == 0) return;
        if (errno != EINVAL || limit.rlim_max <= OPEN_MAX) throw std::system_error(errno, std::generic_category(), "raise open file limit");
        limit.rlim_max = OPEN_MAX;
#endif
        limit.rlim_cur = limit.rlim_max;
        if (setrlimit(RLIMIT_NOFILE, &limit) != 0) throw std::system_error(errno, std::generic_category(), "raise open file limit");
    }
} openFileLimit;

}

#endif
