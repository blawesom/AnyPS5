#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/GuestArena.hpp"
#include "prx/libkernel/File/include/File.hpp"
#include "prx/libkernel/Socket/include/SocketPoll.hpp"
#include "prx/libkernel/Socket/include/SocketRuntime.hpp"
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" int* APS5_VABI __error_nid_postfix();
extern "C" std::int64_t APS5_VABI send_nid_postfix(int descriptor, const void* buffer, std::uint64_t length, int flags);

namespace {

constexpr int GuestEintr = 4;
constexpr int GuestEbadf = 9;
constexpr int GuestEinval = 22;
constexpr int GuestEpipe = 32;
constexpr int GuestEagain = 35;
constexpr int GuestEnotsock = 38;
constexpr int GuestEnobufs = 55;
constexpr int GuestEnotconn = 57;
constexpr int GuestStream = 1;
constexpr int GuestNoSignal = 0x20000;
constexpr unsigned GuestMaxIovecs = 1024;
constexpr int AcceptedFlags = 0x1 | 0x2 | 0x4 | 0x10;
constexpr std::size_t ChunkBytes = 64 * 1024;

struct GuestIovec {
    const void* base;
    std::uint64_t length;
};

struct GuestHeaderTrailer {
    const GuestIovec* headers;
    int headerCount;
    const GuestIovec* trailers;
    int trailerCount;
};
static_assert(sizeof(GuestHeaderTrailer) == 32 && offsetof(GuestHeaderTrailer, trailers) == 16 && offsetof(GuestHeaderTrailer, trailerCount) == 24);

enum class Source { Closed, Unreadable, Other, Regular };

#ifdef _WIN32
using QueryObjectFunction = LONG(NTAPI*)(HANDLE, int, void*, ULONG, ULONG*);
struct ObjectBasicInformation {
    ULONG attributes;
    ACCESS_MASK grantedAccess;
    ULONG handleCount;
    ULONG pointerCount;
    ULONG reserved[10];
};

HANDLE NativeHandle(int descriptor) {
    return reinterpret_cast<HANDLE>(::_get_osfhandle(descriptor));
}

Source Classify(int descriptor) {
    if (!DescriptorIsOpen_nid_no_patch(descriptor)) return Source::Closed;
    static const auto query = reinterpret_cast<QueryObjectFunction>(
        reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryObject")));
    const HANDLE handle = NativeHandle(descriptor);
    ObjectBasicInformation information{};
    if (query == nullptr || query(handle, 0, &information, sizeof(information), nullptr) < 0)
        throw std::runtime_error("sendfile: the access rights of descriptor " + std::to_string(descriptor) + " cannot be read");
    if ((information.grantedAccess & FILE_READ_DATA) == 0) return Source::Unreadable;
    return GetFileType(handle) == FILE_TYPE_DISK ? Source::Regular : Source::Other;
}

std::int64_t FileSize(int descriptor) {
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(NativeHandle(descriptor), &size))
        throw std::runtime_error("sendfile: GetFileSizeEx failed for descriptor " + std::to_string(descriptor) + ", error=" + std::to_string(GetLastError()));
    return size.QuadPart;
}

std::int64_t ReadAt(int descriptor, void* buffer, std::size_t length, std::int64_t offset) {
    return NativePositioned_nid_no_patch(descriptor, buffer, length, offset, false);
}
#else
Source Classify(int descriptor) {
    const int flags = ::fcntl(descriptor, F_GETFL);
    if (flags < 0) return Source::Closed;
    if ((flags & O_ACCMODE) == O_WRONLY) return Source::Unreadable;
    struct stat status{};
    if (::fstat(descriptor, &status) != 0)
        throw std::runtime_error("sendfile: fstat failed for descriptor " + std::to_string(descriptor) + ", errno=" + std::to_string(errno));
    return S_ISREG(status.st_mode) ? Source::Regular : Source::Other;
}

std::int64_t FileSize(int descriptor) {
    struct stat status{};
    if (::fstat(descriptor, &status) != 0)
        throw std::runtime_error("sendfile: fstat failed for descriptor " + std::to_string(descriptor) + ", errno=" + std::to_string(errno));
    return static_cast<std::int64_t>(status.st_size);
}

std::int64_t ReadAt(int descriptor, void* buffer, std::size_t length, std::int64_t offset) {
    return static_cast<std::int64_t>(::pread(descriptor, buffer, length, static_cast<off_t>(offset)));
}
#endif

int Fail(int error) {
    *__error_nid_postfix() = error;
    return -1;
}

int SourceError(int descriptor) {
    if (descriptor >= GuestSockets::FirstDescriptor) return GuestSockets::IsOpen(descriptor) ? GuestEinval : GuestEbadf;
    switch (Classify(descriptor)) {
        case Source::Closed: return KernelSocketPoll::IsNetSocket(descriptor) ? GuestEinval : GuestEbadf;
        case Source::Unreadable: return GuestEbadf;
        case Source::Other: return GuestEinval;
        case Source::Regular: return 0;
    }
    return 0;
}

int SocketError(int descriptor) {
    if (descriptor < GuestSockets::FirstDescriptor) {
        if (KernelSocketPoll::IsNetSocket(descriptor)) NotImplemented_nid_no_patch("sendfile to a libSceNet socket");
        return Classify(descriptor) == Source::Closed ? GuestEbadf : GuestEnotsock;
    }
    const int type = GuestSockets::Type(descriptor);
    if (type < 0) return GuestEbadf;
    if (type != GuestStream) return GuestEinval;
    return GuestSockets::Connected(descriptor) ? 0 : GuestEnotconn;
}

bool VectorBytes(const GuestIovec* vectors, int count, std::uint64_t& total) {
    total = 0;
    if (vectors == nullptr) return true;
    if (static_cast<unsigned>(count) > GuestMaxIovecs) return false;
    for (int i = 0; i < count; ++i) {
        if (vectors[i].length > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) - total) return false;
        total += vectors[i].length;
    }
    return true;
}

int SendAll(int socket, const void* data, std::uint64_t length, std::int64_t& sent) {
    const auto* bytes = static_cast<const char*>(data);
    while (length != 0) {
        const auto result = send_nid_postfix(socket, bytes, std::min<std::uint64_t>(length, INT_MAX), GuestNoSignal);
        if (result < 0) return *__error_nid_postfix();
        if (result == 0) throw std::runtime_error("sendfile: send made no progress");
        sent += result;
        if (bytes != nullptr) bytes += result;
        length -= static_cast<std::uint64_t>(result);
    }
    return 0;
}

int SendVectors(int socket, const GuestIovec* vectors, int count, std::int64_t& sent) {
    if (vectors == nullptr) return 0;
    for (int i = 0; i < count; ++i)
        if (const int error = SendAll(socket, vectors[i].base, vectors[i].length, sent)) return error;
    return 0;
}

void Report(std::int64_t* sbytes, std::int64_t sent) {
    if (sbytes == nullptr) return;
    const GuestArena::HostWrite destination(sbytes, sizeof(*sbytes));
    if (destination.Open()) std::memcpy(sbytes, &sent, sizeof(sent));
}

int Finish(std::int64_t* sbytes, std::int64_t sent, int error) {
    Report(sbytes, sent);
    return error != 0 ? Fail(error) : 0;
}

}

extern "C" int APS5_VABI sendfile_nid_postfix(int fd, int s, std::int64_t offset, std::uint64_t nbytes,
    const GuestHeaderTrailer* hdtr, std::int64_t* sbytes, int flags) {
    if (((flags & 0xffff) & ~AcceptedFlags) != 0)
        throw std::runtime_error("sendfile: unsupported flags " + std::to_string(flags & 0xffff));
    if (offset < 0) return Fail(GuestEinval);
    std::uint64_t headerBytes = 0;
    std::uint64_t trailerBytes = 0;
    if (hdtr != nullptr && (!VectorBytes(hdtr->headers, hdtr->headerCount, headerBytes) ||
        !VectorBytes(hdtr->trailers, hdtr->trailerCount, trailerBytes)))
        return Fail(GuestEinval);
    if (const int error = SourceError(fd)) return error == GuestEbadf ? Fail(error) : Finish(sbytes, 0, error);
    if (const int error = SocketError(s)) return Finish(sbytes, 0, error);
    std::int64_t size = FileSize(fd);
    const auto requested = static_cast<std::int64_t>(nbytes);
    std::int64_t remaining = requested != 0 ? std::min(requested, size - offset) : size - offset;
    if (remaining <= 0 && headerBytes + trailerBytes != 0)
        throw std::runtime_error("sendfile: a header or trailer with no file data to send is not settled by FreeBSD 11");
    std::vector<char> buffer;
    try {
        buffer.resize(static_cast<std::size_t>(std::clamp<std::int64_t>(remaining, 0, ChunkBytes)));
    } catch (const std::bad_alloc&) {
        return Finish(sbytes, 0, GuestEnobufs);
    }
    std::int64_t sent = 0;
    bool headerSent = false;
    for (std::int64_t position = offset; remaining > 0;) {
        const std::int64_t current = FileSize(fd);
        if (position >= current) {
            if (!headerSent && headerBytes != 0)
                throw std::runtime_error("sendfile: a header with the file truncated below the offset is not settled by FreeBSD 11");
            return Finish(sbytes, sent, 0);
        }
        if (current != size) {
            size = current;
            remaining = (requested != 0 && requested <= size - offset ? requested + offset : size) - position;
        }
        if (!headerSent) {
            headerSent = true;
            if (const int error = SendVectors(s, hdtr != nullptr ? hdtr->headers : nullptr, hdtr != nullptr ? hdtr->headerCount : 0, sent))
                return Finish(sbytes, sent, error);
        }
        const auto chunk = static_cast<std::size_t>(std::min<std::int64_t>(remaining, static_cast<std::int64_t>(buffer.size())));
        const auto read = ReadAt(fd, buffer.data(), chunk, position);
        if (read != static_cast<std::int64_t>(chunk))
            throw std::runtime_error("sendfile: reading " + std::to_string(chunk) + " bytes at offset " + std::to_string(position) +
                " of descriptor " + std::to_string(fd) + " returned " + std::to_string(read) + ", errno=" + std::to_string(errno));
        if (const int error = SendAll(s, buffer.data(), chunk, sent)) return Finish(sbytes, sent, error);
        position += static_cast<std::int64_t>(chunk);
        remaining -= static_cast<std::int64_t>(chunk);
    }
    if (hdtr == nullptr || hdtr->trailers == nullptr) return Finish(sbytes, sent, 0);
    std::int64_t trailerSent = 0;
    const int error = SendVectors(s, hdtr->trailers, hdtr->trailerCount, trailerSent);
    if (error == GuestEpipe) throw std::runtime_error("sendfile: SIGPIPE for a trailer write is not delivered");
    if (error == 0 || ((error == GuestEagain || error == GuestEintr) && trailerSent != 0)) return Finish(sbytes, sent + trailerSent, 0);
    return Finish(sbytes, sent, error);
}
