#include "SceTypes.hpp"

#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

extern "C" {
using GuestRudpEventHandler = void(APS5_VABI*)(int, int, int, void*);
int APS5_VABI sceRudpInit_nid_postfix(void*, int);
int APS5_VABI sceRudpGetStatus(void*, std::size_t);
int APS5_VABI sceRudpCreateContext(GuestRudpEventHandler, void*, int*);
int APS5_VABI sceRudpTerminate(void);
int APS5_VABI sceRudpEnd(int);
int APS5_VABI sceRudpEnableInternalIOThread(std::uint32_t, std::uint32_t);
int APS5_VABI sceRudpBind(int, std::uint16_t, int, std::uint16_t);
int APS5_VABI sceRudpInitiate(int, const void*, std::uint32_t, std::uint16_t);
int APS5_VABI sceRudpGetMaxSegmentSize(std::uint16_t*);
int APS5_VABI sceRudpProcessEvents(std::uint64_t);
int APS5_VABI sceRudpNetFlush();
int APS5_VABI sceRudpNetReceived(int, const std::uint8_t*, std::size_t, const void*, std::uint32_t);
int APS5_VABI sceRudpGetNumberOfPacketsToRead(int);
int APS5_VABI sceRudpGetSizeWritable(int);
int APS5_VABI sceRudpGetSizeReadable(int);
}

static void Require(bool value) { if (!value) std::abort(); }

template <typename TCall>
static void RequireNotImplemented(const char* name, TCall call) {
    bool thrown = false;
    try {
        call();
    } catch (const std::runtime_error& error) {
        thrown = std::string(error.what()) == std::string(name) + " not implemented";
    }
    Require(thrown);
}

static void RequireQueries(int ctx, int packets, int size) {
    Require(sceRudpGetNumberOfPacketsToRead(ctx) == packets);
    Require(sceRudpGetSizeWritable(ctx) == size);
    Require(sceRudpGetSizeReadable(ctx) == size);
}

static void RequireLibrary(int result) {
    std::uint16_t mss = 0x5a5a;
    Require(sceRudpGetMaxSegmentSize(&mss) == result);
    Require(mss == 0x5a5a);
    Require(sceRudpGetMaxSegmentSize(nullptr) == result);
    Require(sceRudpProcessEvents(0) == result);
    Require(sceRudpProcessEvents(1000) == result);
    Require(sceRudpNetFlush() == result);
}

static void APS5_VABI Handler(int, int, int, void*) {}

int main() {
    constexpr int notInitialized = static_cast<int>(0x80770001);
    constexpr int alreadyInitialized = static_cast<int>(0x80770002);
    constexpr int invalidContextId = static_cast<int>(0x80770003);
    constexpr int invalidArgument = static_cast<int>(0x80770004);
    constexpr int memory = static_cast<int>(0x80770007);
    constexpr int notAcceptable = static_cast<int>(0x80770011);
    constexpr std::uint16_t maxSegmentSize = 1410;
    constexpr std::size_t statusSize = 0xF8;
    constexpr std::size_t currentContexts = 0x64;
    alignas(16) static unsigned char pool[0x10000];

    std::array<unsigned char, 0x200> status;
    status.fill(0x5a);
    const auto untouched = status;
    Require(sceRudpGetStatus(status.data(), statusSize) == notInitialized);
    Require(sceRudpGetStatus(nullptr, 0) == notInitialized);
    Require(sceRudpGetStatus(status.data(), statusSize + 1) == notInitialized);
    Require(status == untouched);
    Require(sceRudpEnd(0) == notInitialized);
    Require(sceRudpTerminate() == notInitialized);
    RequireLibrary(notInitialized);
    for (int ctx : {0, 12345, -1}) RequireQueries(ctx, notInitialized, notInitialized);
    const std::uint8_t datagram[4] = {};
    RequireNotImplemented("sceRudpNetReceived", [&] { sceRudpNetReceived(0, datagram, sizeof(datagram), nullptr, 0); });

    Require(sceRudpInit_nid_postfix(nullptr, sizeof(pool)) == invalidArgument);
    Require(sceRudpInit_nid_postfix(pool, 0) == invalidArgument);
    Require(sceRudpInit_nid_postfix(pool, -1) == invalidArgument);
    Require(sceRudpInit_nid_postfix(pool, 0xC57) == memory);
    for (std::size_t offset : {1u, 2u, 4u}) Require(sceRudpInit_nid_postfix(pool + offset, 0x1000) == memory);
    Require(sceRudpInit_nid_postfix(pool + 8, 0xC58) == 0);
    Require(sceRudpInit_nid_postfix(nullptr, 0) == alreadyInitialized);

    Require(sceRudpGetStatus(nullptr, statusSize) == invalidArgument);
    Require(sceRudpGetStatus(nullptr, 0) == invalidArgument);
    Require(sceRudpGetStatus(status.data(), 0) == invalidArgument);
    Require(sceRudpGetStatus(status.data(), statusSize + 1) == invalidArgument);
    Require(status == untouched);

    Require(sceRudpGetStatus(status.data(), statusSize) == 0);
    for (std::size_t i = 0; i < statusSize; ++i) Require(status[i] == 0);
    for (std::size_t i = statusSize; i < status.size(); ++i) Require(status[i] == 0x5a);

    for (std::uint16_t initial : {std::uint16_t{0}, std::uint16_t{0x5a5a}}) {
        std::uint16_t mss = initial;
        Require(sceRudpGetMaxSegmentSize(&mss) == 0);
        Require(mss == maxSegmentSize);
    }
    Require(sceRudpGetMaxSegmentSize(nullptr) == invalidArgument);
    Require(sceRudpProcessEvents(0) == 0);
    Require(sceRudpProcessEvents(1000) == 0);
    Require(sceRudpNetFlush() == 0);
    for (int ctx : {0, 1, 12345}) RequireQueries(ctx, invalidContextId, invalidContextId);
    for (int ctx : {-1, INT_MIN}) RequireQueries(ctx, invalidArgument, invalidArgument);

    int first = -1;
    int second = -1;
    Require(sceRudpCreateContext(Handler, nullptr, &first) == 0);
    Require(sceRudpCreateContext(Handler, nullptr, &second) == 0);
    status.fill(0x5a);
    Require(sceRudpGetStatus(status.data(), statusSize) == 0);
    std::uint32_t contexts = 0;
    std::memcpy(&contexts, status.data() + currentContexts, sizeof(contexts));
    Require(contexts == 2);
    for (std::size_t i = 0; i < statusSize; ++i) {
        if (i < currentContexts || i >= currentContexts + sizeof(contexts)) Require(status[i] == 0);
    }

    status.fill(0x5a);
    Require(sceRudpGetStatus(status.data(), 1) == 0);
    Require(status[0] == 0);
    for (std::size_t i = 1; i < status.size(); ++i) Require(status[i] == 0x5a);

    RequireQueries(first, 0, notAcceptable);
    RequireQueries(second, 0, notAcceptable);
    RequireQueries(12345, invalidContextId, invalidContextId);
    RequireQueries(-1, invalidArgument, invalidArgument);
    Require(sceRudpBind(second, 1, 0, 0) == 0);
    Require(sceRudpInitiate(second, nullptr, 0, 1) == static_cast<int>(0x8077000A));
    RequireQueries(second, 0, notAcceptable);

    Require(sceRudpEnd(first) == 0);
    Require(sceRudpEnd(first) == invalidContextId);
    RequireQueries(first, invalidContextId, invalidContextId);
    RequireQueries(second, 0, notAcceptable);
    Require(sceRudpGetStatus(status.data(), statusSize) == 0);
    std::memcpy(&contexts, status.data() + currentContexts, sizeof(contexts));
    Require(contexts == 1);

    Require(sceRudpEnableInternalIOThread(0x4000, 0x300) == 0);
    std::uint16_t mss = 0;
    Require(sceRudpGetMaxSegmentSize(&mss) == 0);
    Require(mss == maxSegmentSize);
    RequireNotImplemented("sceRudpProcessEvents", [] { sceRudpProcessEvents(0); });
    RequireNotImplemented("sceRudpNetFlush", [] { sceRudpNetFlush(); });

    Require(sceRudpTerminate() == 0);
    Require(sceRudpTerminate() == notInitialized);
    RequireLibrary(notInitialized);
    RequireQueries(second, notInitialized, notInitialized);
    Require(sceRudpEnd(second) == notInitialized);
    Require(sceRudpGetStatus(status.data(), statusSize) == notInitialized);
    Require(sceRudpInit_nid_postfix(pool + 8, 0xC58) == 0);
    Require(sceRudpEnd(second) == invalidContextId);
    RequireQueries(second, invalidContextId, invalidContextId);
    Require(sceRudpProcessEvents(0) == 0);
    Require(sceRudpNetFlush() == 0);
    Require(sceRudpTerminate() == 0);
    return 0;
}
