#ifndef CORE_LIBS_PRX_LIBC_INCLUDE_SPECIFICS_MACOS_POSIXMEMORY_HPP
#define CORE_LIBS_PRX_LIBC_INCLUDE_SPECIFICS_MACOS_POSIXMEMORY_HPP

#if defined(__linux__) || defined(__APPLE__)

#include <sys/mman.h>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#if defined(__APPLE__)
#include <atomic>
#include <cstdio>
#include <fcntl.h>
#include <cstdlib>
#include <unistd.h>
#include "prx/libc/include/specifics/macos/HostRegions.hpp"
#else
#include <fstream>
#include <sstream>
#include <string>
#endif

namespace PosixMemory {

inline void* MapNoReplace(void* address, std::size_t bytes, int protection, int flags, int file, off_t offset) {
#if defined(__APPLE__)
    void* placed = ::mmap(address, bytes, protection, flags & ~MAP_FIXED, file, offset);
    if (placed == MAP_FAILED || placed == address) return placed;
    ::munmap(placed, bytes);
    errno = EEXIST;
    return MAP_FAILED;
#else
    return ::mmap(address, bytes, protection, flags | MAP_FIXED_NOREPLACE, file, offset);
#endif
}

inline int CreateAnonymousFile(const char* name) {
#if defined(__APPLE__)
    static std::atomic<unsigned> serial{0};
    for (int attempt = 0; attempt < 64; ++attempt) {
        char path[64];
        std::snprintf(path, sizeof(path), "/aps5.%d.%u", static_cast<int>(::getpid()), serial.fetch_add(1, std::memory_order_relaxed));
        const int file = ::shm_open(path, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (file < 0) {
            if (errno == EEXIST) continue;
            return -1;
        }
        ::shm_unlink(path);
        ::fcntl(file, F_SETFD, FD_CLOEXEC);
        static_cast<void>(name);
        return file;
    }
    errno = EEXIST;
    return -1;
#else
    return ::memfd_create(name, MFD_CLOEXEC | MFD_ALLOW_SEALING);
#endif
}

inline int CreateResizableFile(const char* name) {
#if defined(__APPLE__)
    static_cast<void>(name);
    const char* directory = ::getenv("TMPDIR");
    char path[1024];
    std::snprintf(path, sizeof(path), "%s/aps5.XXXXXX", directory != nullptr && *directory != '\0' ? directory : "/tmp");
    const int file = ::mkstemp(path);
    if (file < 0) return -1;
    ::unlink(path);
    ::fcntl(file, F_SETFD, FD_CLOEXEC);
    return file;
#else
    return CreateAnonymousFile(name);
#endif
}

inline std::vector<std::pair<std::uintptr_t, std::uintptr_t>> HostMappedRanges() {
    std::vector<std::pair<std::uintptr_t, std::uintptr_t>> ranges;
#if defined(__APPLE__)
    std::uintptr_t cursor = 0;
    HostRegions::Region region;
    while (HostRegions::FirstAtOrAbove(cursor, region) && region.end > cursor) {
        ranges.emplace_back(region.begin, region.end);
        cursor = region.end;
    }
#else
    std::ifstream maps("/proc/self/maps");
    std::string line;
    while (std::getline(maps, line)) {
        std::istringstream fields(line);
        std::uintptr_t begin = 0, end = 0;
        char dash = 0;
        if (fields >> std::hex >> begin >> dash >> end && dash == '-' && end > begin) ranges.emplace_back(begin, end);
    }
#endif
    return ranges;
}

}

#endif

#endif
