#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#ifndef _WIN32
#include <sys/resource.h>
#endif

struct ResourceLimit {
    std::int64_t rlim_cur;
    std::int64_t rlim_max;
};

extern "C" int APS5_VABI getrlimit_nid_postfix(int resource, ResourceLimit* limit);
extern "C" int APS5_VABI setrlimit_nid_postfix(int resource, const ResourceLimit* limit);
extern "C" int* APS5_VABI __error_nid_postfix();

static constexpr int GUEST_EFAULT = 14;
static constexpr int GUEST_EINVAL = 22;
static constexpr int GUEST_RLIMIT_DATA = 2;
static constexpr int GUEST_RLIMIT_NOFILE = 8;
static constexpr int GUEST_RLIMIT_AS = 10;
static constexpr int GUEST_RLIM_NLIMITS = 15;
static constexpr std::int64_t GUEST_RLIM_INFINITY = std::numeric_limits<std::int64_t>::max();

static void Require(bool value) { if (!value) std::abort(); }

static ResourceLimit GuestLimit(int resource) {
    ResourceLimit limit{-1, -1};
    Require(getrlimit_nid_postfix(resource, &limit) == 0);
    Require(limit.rlim_cur >= 0 && limit.rlim_cur <= limit.rlim_max);
    return limit;
}

static void RequireError(int resource, ResourceLimit* limit, int error) {
    *__error_nid_postfix() = 0;
    Require(getrlimit_nid_postfix(resource, limit) == -1);
    Require(*__error_nid_postfix() == error);
}

#ifndef _WIN32
static std::int64_t Guest(rlim_t value) {
    return value == RLIM_INFINITY ? GUEST_RLIM_INFINITY : static_cast<std::int64_t>(value);
}

static void RequireHostLimit(int resource, int hostResource) {
    rlimit host{};
    Require(getrlimit(hostResource, &host) == 0);
    const ResourceLimit guest = GuestLimit(resource);
    Require(guest.rlim_cur == Guest(host.rlim_cur));
    Require(guest.rlim_max == Guest(host.rlim_max));
}

static void LoweredHostLimitIsReported() {
    rlimit host{};
    Require(getrlimit(RLIMIT_DATA, &host) == 0);
    const rlim_t lowered = rlim_t{1} << 40;
    if (host.rlim_max != RLIM_INFINITY && host.rlim_max < lowered) return;
    host.rlim_cur = lowered;
    Require(setrlimit(RLIMIT_DATA, &host) == 0);
    Require(GuestLimit(GUEST_RLIMIT_DATA).rlim_cur == static_cast<std::int64_t>(lowered));
}
#endif

static void RequireSetError(int resource, const ResourceLimit* limit, int error) {
    *__error_nid_postfix() = 0;
    Require(setrlimit_nid_postfix(resource, limit) == -1);
    Require(*__error_nid_postfix() == error);
}

static bool SetThrows(int resource, ResourceLimit limit) {
    try {
        setrlimit_nid_postfix(resource, &limit);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

static void CurrentLimitIsKept() {
    const ResourceLimit current = GuestLimit(GUEST_RLIMIT_DATA);
    *__error_nid_postfix() = 13;
    Require(setrlimit_nid_postfix(GUEST_RLIMIT_DATA, &current) == 0);
    Require(*__error_nid_postfix() == 13);
    if (current.rlim_cur == current.rlim_max) {
        const ResourceLimit above{GUEST_RLIM_INFINITY, current.rlim_max};
        Require(setrlimit_nid_postfix(GUEST_RLIMIT_DATA, &above) == 0);
        Require(SetThrows(GUEST_RLIMIT_DATA, {GUEST_RLIM_INFINITY, current.rlim_max - 1}));
    }
    if (current.rlim_cur == GUEST_RLIM_INFINITY && current.rlim_max == GUEST_RLIM_INFINITY) {
        const ResourceLimit negative{-1, -2};
        Require(setrlimit_nid_postfix(GUEST_RLIMIT_DATA, &negative) == 0);
    }
    if (current.rlim_cur > 0) Require(SetThrows(GUEST_RLIMIT_DATA, {current.rlim_cur - 1, current.rlim_max}));
    if (current.rlim_max < GUEST_RLIM_INFINITY) Require(SetThrows(GUEST_RLIMIT_DATA, {current.rlim_cur, GUEST_RLIM_INFINITY}));
    const ResourceLimit after = GuestLimit(GUEST_RLIMIT_DATA);
    Require(after.rlim_cur == current.rlim_cur && after.rlim_max == current.rlim_max);
}

static void RequireUnsupported(int resource) {
    ResourceLimit limit{};
    bool rejected = false;
    try {
        getrlimit_nid_postfix(resource, &limit);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    Require(rejected);
}

int main() {
#ifdef _WIN32
    GuestLimit(GUEST_RLIMIT_DATA);
#else
    RequireHostLimit(GUEST_RLIMIT_DATA, RLIMIT_DATA);
    LoweredHostLimitIsReported();
#endif
    ResourceLimit limit{};
    RequireError(GUEST_RLIM_NLIMITS, &limit, GUEST_EINVAL);
    RequireError(-1, &limit, GUEST_EINVAL);
    RequireError(GUEST_RLIM_NLIMITS, nullptr, GUEST_EINVAL);
    RequireError(GUEST_RLIMIT_DATA, nullptr, GUEST_EFAULT);
    RequireUnsupported(GUEST_RLIMIT_NOFILE);
    RequireUnsupported(GUEST_RLIMIT_AS);

    CurrentLimitIsKept();
    const ResourceLimit infinite{GUEST_RLIM_INFINITY, GUEST_RLIM_INFINITY};
    RequireSetError(GUEST_RLIMIT_DATA, nullptr, GUEST_EFAULT);
    RequireSetError(GUEST_RLIM_NLIMITS, nullptr, GUEST_EFAULT);
    RequireSetError(GUEST_RLIM_NLIMITS, &infinite, GUEST_EINVAL);
    RequireSetError(-1, &infinite, GUEST_EINVAL);
    Require(SetThrows(GUEST_RLIMIT_NOFILE, infinite));
    Require(SetThrows(GUEST_RLIMIT_AS, infinite));
}
