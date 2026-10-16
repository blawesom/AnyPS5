#include "prx/libSceAgcDriver/Execution/include/Driver/Driver.hpp"
#include "prx/libSceAgcDriver/Execution/include/Driver/Diagnostics.hpp"
#include <cstdlib>
#include <stdexcept>
#if defined(__APPLE__)
#include <algorithm>
#include <array>
#include <cstdio>
#include <mutex>
#endif

namespace AgcDriver::DriverDetail {

double TraceMs() {
    static const auto origin = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - origin).count();
}

std::uint64_t SkippedShader() {
    static const std::uint64_t skipped = [] { const char* text = std::getenv("APS5_SKIP_SHADER"); return text ? std::strtoull(text, nullptr, 16) : 0ull; }();
    return skipped;
}

#if defined(__APPLE__)
namespace {

struct RecentWork {
    std::uint64_t serial = 0;
    double ms = 0;
    char kind = 0;
    std::uint32_t queue = 0;
    std::array<std::uint64_t, 4> shaders{};
    std::uint64_t target = 0;
};

struct RecentWorkLog {
    std::mutex mutex;
    std::array<RecentWork, 32> entries{};
    std::uint64_t next = 0;
};

RecentWorkLog& RecentWorkEntries() {
    static auto* log = new RecentWorkLog();
    return *log;
}

}

void NoteRecentWork(char kind, std::uint32_t queue, std::span<const std::uint64_t> shaders, std::uint64_t target) {
    auto& log = RecentWorkEntries();
    std::lock_guard lock(log.mutex);
    auto& entry = log.entries[log.next % log.entries.size()];
    entry = {};
    entry.serial = log.next++;
    entry.ms = TraceMs();
    entry.kind = kind;
    entry.queue = queue;
    for (std::size_t i = 0; i < shaders.size() && i < entry.shaders.size(); ++i) entry.shaders[i] = shaders[i];
    entry.target = target;
}

void PrintRecentWork() {
    auto& log = RecentWorkEntries();
    std::lock_guard lock(log.mutex);
    const auto count = std::min<std::uint64_t>(log.next, log.entries.size());
    std::fprintf(stderr, "[gpu] last %llu draws and dispatches before the failure (oldest first; APS5_SKIP_SHADER takes one of the shader addresses):\n", static_cast<unsigned long long>(count));
    for (auto serial = log.next - count; serial < log.next; ++serial) {
        const auto& entry = log.entries[serial % log.entries.size()];
        std::fprintf(stderr, "[gpu]   #%llu %.1f ms %s queue=0x%x target=0x%llx shaders=", static_cast<unsigned long long>(entry.serial), entry.ms, entry.kind == 'd' ? "draw" : "dispatch", entry.queue, static_cast<unsigned long long>(entry.target));
        for (const auto shader : entry.shaders) if (shader != 0) std::fprintf(stderr, "0x%llx ", static_cast<unsigned long long>(shader));
        std::fprintf(stderr, "\n");
    }
    std::fflush(stderr);
}
#else
void NoteRecentWork(char, std::uint32_t, std::span<const std::uint64_t>, std::uint64_t) {}

void PrintRecentWork() {}
#endif

void require(bool condition, const char* reason) {
    if (!condition) {
        throw std::runtime_error(std::string("AGC driver: ") + reason);
    }
}

std::uint32_t readRegister(const Registers& registers, std::uint32_t offset) {
    const auto it = registers.find(offset);
    require(it != registers.end(), "required shader register has not been written");
    return it->second;
}

std::uint32_t readUserData(const Registers& shader, std::uint32_t offset) {
    const auto it = shader.find(offset);
    return it == shader.end() ? 0u : it->second;
}

}
