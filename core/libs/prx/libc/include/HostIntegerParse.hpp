#ifndef CORE_LIBS_PRX_LIBC_INCLUDE_HOSTINTEGERPARSE_HPP
#define CORE_LIBS_PRX_LIBC_INCLUDE_HOSTINTEGERPARSE_HPP

#include <cerrno>
#include <cinttypes>
#include <cstdint>
#include <cstdlib>

template<typename TValue>
TValue PreserveErrnoWithoutConversion(const char* text, char** end, int base, TValue (*parse)(const char*, char**, int)) {
#if defined(__APPLE__)
    const int saved = errno;
    char* stop = nullptr;
    const TValue value = parse(text, &stop, base);
    if (stop == text && errno == EINVAL && (base == 0 || (base >= 2 && base <= 36))) errno = saved;
    if (end != nullptr) *end = stop;
    return value;
#else
    return parse(text, end, base);
#endif
}

inline long long HostStrtoll(const char* text, char** end, int base) {
    return PreserveErrnoWithoutConversion<long long>(text, end, base, std::strtoll);
}

inline unsigned long long HostStrtoull(const char* text, char** end, int base) {
    return PreserveErrnoWithoutConversion<unsigned long long>(text, end, base, std::strtoull);
}

inline std::intmax_t HostStrtoimax(const char* text, char** end, int base) {
    return PreserveErrnoWithoutConversion<std::intmax_t>(text, end, base, std::strtoimax);
}

inline std::uintmax_t HostStrtoumax(const char* text, char** end, int base) {
    return PreserveErrnoWithoutConversion<std::uintmax_t>(text, end, base, std::strtoumax);
}

#endif
