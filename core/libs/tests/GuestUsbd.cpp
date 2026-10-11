#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

namespace {

struct UsbdTimeval {
    std::int64_t seconds;
    std::int64_t microseconds;
};

struct UsbdTransfer;

using UsbdTransferCallback = void (APS5_VABI *)(UsbdTransfer*);

struct alignas(8) UsbdIsoPacketDescriptor {
    std::uint32_t length;
    std::uint32_t actualLength;
    std::int32_t status;
};

struct alignas(8) UsbdTransfer {
    void* deviceHandle;
    std::uint8_t flags;
    std::uint8_t endpoint;
    std::uint8_t type;
    std::uint32_t timeout;
    std::int32_t status;
    std::int32_t length;
    std::int32_t actualLength;
    UsbdTransferCallback callback;
    void* userData;
    std::uint8_t* buffer;
    std::int32_t isoPacketCount;
};

}

extern "C" {
std::int32_t APS5_VABI sceUsbdInit();
void APS5_VABI sceUsbdExit();
std::int64_t APS5_VABI sceUsbdGetDeviceList(void*** list);
void APS5_VABI sceUsbdFreeDeviceList(void** list, std::int32_t unrefDevices);
std::int32_t APS5_VABI sceUsbdHandleEventsTimeout(const UsbdTimeval* timeout);
UsbdTransfer* APS5_VABI sceUsbdAllocTransfer(std::int32_t isoPackets);
void APS5_VABI sceUsbdFreeTransfer(UsbdTransfer* transfer);
void APS5_VABI sceUsbdFillInterruptTransfer(UsbdTransfer* transfer, void* handle, std::uint8_t endpoint, std::uint8_t* buffer, std::int32_t length, UsbdTransferCallback callback, void* userData, std::uint32_t timeout);
std::uint8_t APS5_VABI sceUsbdGetBusNumber(void* device);
std::uint8_t APS5_VABI sceUsbdGetDeviceAddress(void* device);
void* APS5_VABI sceUsbdRefDevice(void* device);
void APS5_VABI sceUsbdUnrefDevice(void* device);
std::int32_t APS5_VABI sceUsbdGetDeviceDescriptor(void* device, void* descriptor);
std::int32_t APS5_VABI sceUsbdGetConfigDescriptor(void* device, std::uint8_t configIndex, void** config);
std::int32_t APS5_VABI sceUsbdGetActiveConfigDescriptor(void* device, void** config);
void APS5_VABI sceUsbdFreeConfigDescriptor(void* config);
std::int32_t APS5_VABI sceUsbdOpen(void* device, void** handle);
}

namespace {

constexpr std::int32_t invalidArgument = static_cast<std::int32_t>(0x80240002);

std::size_t releases = 0;
void* lastRelease = nullptr;

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "USBD: %s\n", message);
        std::abort();
    }
}

template<typename TCall>
void RequireThrows(TCall call, const char* message) {
    bool threw = false;
    try {
        call();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw, message);
}

void* APS5_VABI allocate(std::size_t bytes) {
    return std::malloc(bytes);
}

void APS5_VABI release(void* pointer) {
    ++releases;
    lastRelease = pointer;
    std::free(pointer);
}

void* APS5_VABI allocateZeroed(std::size_t, std::size_t) { std::abort(); }
void* APS5_VABI reallocate(void*, std::size_t) { std::abort(); }
void* APS5_VABI align(std::size_t, std::size_t) { std::abort(); }
void* APS5_VABI realign(void*, std::size_t, std::size_t) { std::abort(); }
int APS5_VABI posixAlign(void**, std::size_t, std::size_t) { std::abort(); }

void APS5_VABI completed(UsbdTransfer*) {
    std::abort();
}

void TestTransfers() {
    UsbdTransfer* transfer = sceUsbdAllocTransfer(0);
    Require(transfer != nullptr, "transfer allocation failed");
    Require(reinterpret_cast<std::uintptr_t>(transfer) % alignof(UsbdTransfer) == 0, "transfer is misaligned");
    Require(transfer->deviceHandle == nullptr && transfer->flags == 0 && transfer->endpoint == 0 && transfer->type == 0 && transfer->timeout == 0, "allocated transfer header is not zeroed");
    Require(transfer->status == 0 && transfer->length == 0 && transfer->actualLength == 0 && transfer->callback == nullptr && transfer->userData == nullptr && transfer->buffer == nullptr && transfer->isoPacketCount == 0, "allocated transfer is not zeroed");

    std::array<std::uint8_t, 8> data{};
    int userData = 0;
    transfer->flags = 1;
    transfer->status = 3;
    transfer->actualLength = 5;
    sceUsbdFillInterruptTransfer(transfer, nullptr, 0x81, data.data(), static_cast<std::int32_t>(data.size()), completed, &userData, 250);
    Require(transfer->deviceHandle == nullptr && transfer->endpoint == 0x81 && transfer->type == 3 && transfer->timeout == 250, "interrupt transfer header not filled");
    Require(transfer->buffer == data.data() && transfer->length == 8 && transfer->callback == completed && transfer->userData == &userData, "interrupt transfer payload not filled");
    Require(transfer->flags == 1 && transfer->status == 3 && transfer->actualLength == 5 && transfer->isoPacketCount == 0, "fill changed fields it does not own");

    int foreignHandle = 0;
    RequireThrows([&] { sceUsbdFillInterruptTransfer(transfer, &foreignHandle, 0x02, nullptr, 0, nullptr, nullptr, 0); }, "transfer filled with a handle sceUsbdOpen did not return");
    Require(transfer->deviceHandle == nullptr && transfer->endpoint == 0x81 && transfer->buffer == data.data(), "rejected fill changed the transfer");
    RequireThrows([] { sceUsbdFillInterruptTransfer(nullptr, nullptr, 0, nullptr, 0, nullptr, nullptr, 0); }, "null transfer filled");

    const std::size_t releasesBefore = releases;
    sceUsbdFreeTransfer(transfer);
    Require(releases == releasesBefore, "buffer freed without the free-buffer flag");

    UsbdTransfer* iso = sceUsbdAllocTransfer(4);
    Require(iso != nullptr && iso->isoPacketCount == 4, "iso transfer allocation failed");
    auto* descriptors = reinterpret_cast<UsbdIsoPacketDescriptor*>(iso + 1);
    for (int index = 0; index < 4; ++index) {
        Require(descriptors[index].length == 0 && descriptors[index].actualLength == 0 && descriptors[index].status == 0, "iso packet descriptors are not zeroed");
        descriptors[index].length = 64;
    }
    auto* owned = static_cast<std::uint8_t*>(ApplicationHeapAllocate_nid_no_patch(16));
    iso->flags = 2;
    iso->buffer = owned;
    sceUsbdFreeTransfer(iso);
    Require(releases == releasesBefore + 1 && lastRelease == owned, "free-buffer flag did not free the buffer through the title's heap");

    sceUsbdFreeTransfer(nullptr);
    UsbdTransfer foreign{};
    RequireThrows([&] { sceUsbdFreeTransfer(&foreign); }, "foreign transfer freed");
    RequireThrows([] { sceUsbdAllocTransfer(-1); }, "negative iso packet count accepted");
}

void TestDevices() {
    int foreign = 0;
    Require(sceUsbdRefDevice(nullptr) == nullptr, "null device referenced");
    sceUsbdUnrefDevice(nullptr);
    Require(sceUsbdGetBusNumber(nullptr) == 0, "null device has a bus number");
    Require(sceUsbdGetDeviceAddress(nullptr) == 0, "null device has an address");
    std::array<std::uint8_t, 18> descriptor{};
    Require(sceUsbdGetDeviceDescriptor(nullptr, descriptor.data()) == invalidArgument, "null device described");
    void* config = &foreign;
    Require(sceUsbdGetConfigDescriptor(nullptr, 0, &config) == invalidArgument && config == &foreign, "null device configuration returned");
    sceUsbdFreeConfigDescriptor(nullptr);

    RequireThrows([&] { sceUsbdRefDevice(&foreign); }, "unlisted device referenced");
    RequireThrows([&] { sceUsbdUnrefDevice(&foreign); }, "unlisted device unreferenced");
    RequireThrows([&] { sceUsbdGetBusNumber(&foreign); }, "unlisted device has a bus number");
    RequireThrows([&] { sceUsbdGetDeviceAddress(&foreign); }, "unlisted device has an address");
    RequireThrows([&] { sceUsbdGetDeviceDescriptor(&foreign, descriptor.data()); }, "unlisted device described");
    RequireThrows([&] { sceUsbdGetDeviceDescriptor(&foreign, nullptr); }, "unlisted device described into a null descriptor");
    RequireThrows([&] { sceUsbdGetConfigDescriptor(&foreign, 0, &config); }, "unlisted device configuration returned");
    RequireThrows([&] { sceUsbdGetActiveConfigDescriptor(&foreign, &config); }, "unlisted device active configuration returned");
    RequireThrows([&] { sceUsbdGetActiveConfigDescriptor(nullptr, &config); }, "null device active configuration returned");
    RequireThrows([&] { sceUsbdFreeConfigDescriptor(&foreign); }, "foreign config descriptor freed");
    void* handle = &foreign;
    RequireThrows([&] { sceUsbdOpen(&foreign, &handle); }, "unlisted device opened");
    RequireThrows([&] { sceUsbdOpen(nullptr, &handle); }, "null device opened");
    Require(handle == &foreign && config == &foreign, "rejected calls wrote their outputs");
}

}

int main() {
    const std::array<void*, 10> api{reinterpret_cast<void*>(&allocate), reinterpret_cast<void*>(&release), reinterpret_cast<void*>(&allocateZeroed), reinterpret_cast<void*>(&reallocate), reinterpret_cast<void*>(&align), reinterpret_cast<void*>(&realign), reinterpret_cast<void*>(&posixAlign)};
    ApplicationHeapRegister_nid_no_patch(api.data());

    Require(sceUsbdInit() == 0, "initialization failed");
    void** list = nullptr;
    Require(sceUsbdGetDeviceList(&list) == 0, "a device was listed");
    Require(list != nullptr && list[0] == nullptr, "device list is not an empty null-terminated array");
    sceUsbdFreeDeviceList(list, 1);
    Require(sceUsbdGetDeviceList(nullptr) == invalidArgument, "null list accepted");
    Require(sceUsbdHandleEventsTimeout(nullptr) == invalidArgument, "null timeout accepted");
    const UsbdTimeval invalid{0, 1000000};
    Require(sceUsbdHandleEventsTimeout(&invalid) == invalidArgument, "out-of-range microseconds accepted");
    const UsbdTimeval timeout{0, 50000};
    const auto start = std::chrono::steady_clock::now();
    Require(sceUsbdHandleEventsTimeout(&timeout) == 0, "event handling failed");
    Require(std::chrono::steady_clock::now() - start >= std::chrono::milliseconds(45), "event handling returned before the timeout");
    TestTransfers();
    TestDevices();
    sceUsbdExit();
    std::puts("USBD tests passed");
    return 0;
}
