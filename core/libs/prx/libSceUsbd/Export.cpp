#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/ApplicationHeap.hpp"

#include <chrono>
#include <climits>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>

namespace {

constexpr std::int32_t SCE_USBD_ERROR_INVALID_ARG = static_cast<std::int32_t>(0x80240002);

constexpr std::uint8_t TRANSFER_FREE_BUFFER = 1 << 1;
constexpr std::uint8_t TRANSFER_TYPE_INTERRUPT = 3;

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

static_assert(sizeof(UsbdIsoPacketDescriptor) == 16);
static_assert(offsetof(UsbdTransfer, flags) == 8);
static_assert(offsetof(UsbdTransfer, timeout) == 12);
static_assert(offsetof(UsbdTransfer, status) == 16);
static_assert(offsetof(UsbdTransfer, callback) == 32);
static_assert(offsetof(UsbdTransfer, buffer) == 48);
static_assert(offsetof(UsbdTransfer, isoPacketCount) == 56);
static_assert(sizeof(UsbdTransfer) == 64);

void* g_emptyDeviceList[1] = {nullptr};

std::mutex g_transferMutex;
std::unordered_map<UsbdTransfer*, std::unique_ptr<std::byte[]>> g_transfers;

std::string ForeignDevice(const char* function) {
    return std::string(function) + ": the device was not returned by sceUsbdGetDeviceList, which lists no devices";
}

std::string ForeignHandle(const char* function) {
    return std::string(function) + ": the device handle was not returned by sceUsbdOpen, which opens no devices";
}

}

extern "C" {

std::int32_t APS5_VABI sceUsbdInit() {
    return 0;
}

void APS5_VABI sceUsbdExit() {
}

std::int64_t APS5_VABI sceUsbdGetDeviceList(void*** list) {
    if (list == nullptr) return SCE_USBD_ERROR_INVALID_ARG;
    *list = g_emptyDeviceList;
    return 0;
}

void APS5_VABI sceUsbdFreeDeviceList(void** list, std::int32_t unrefDevices) {
    (void)unrefDevices;
    if (list != nullptr && list != g_emptyDeviceList) throw std::runtime_error(std::string(__func__) + ": list was not returned by sceUsbdGetDeviceList");
}

std::int32_t APS5_VABI sceUsbdHandleEventsTimeout(const UsbdTimeval* timeout) {
    if (timeout == nullptr || timeout->seconds < 0 || timeout->microseconds < 0 || timeout->microseconds >= 1000000) return SCE_USBD_ERROR_INVALID_ARG;
    std::this_thread::sleep_for(std::chrono::seconds(timeout->seconds) + std::chrono::microseconds(timeout->microseconds));
    return 0;
}

UsbdTransfer* APS5_VABI sceUsbdAllocTransfer(std::int32_t isoPackets) {
    if (isoPackets < 0) throw std::runtime_error(std::string(__func__) + ": negative iso packet count " + std::to_string(isoPackets));
    const std::size_t bytes = sizeof(UsbdTransfer) + static_cast<std::size_t>(isoPackets) * sizeof(UsbdIsoPacketDescriptor);
    if (bytes > static_cast<std::size_t>(INT_MAX)) throw std::runtime_error(std::string(__func__) + ": iso packet count " + std::to_string(isoPackets) + " overflows the transfer size");
    std::unique_ptr<std::byte[]> storage(new (std::nothrow) std::byte[bytes]());
    if (storage == nullptr) return nullptr;
    auto* transfer = new (storage.get()) UsbdTransfer{};
    transfer->isoPacketCount = isoPackets;
    std::lock_guard lock(g_transferMutex);
    g_transfers.emplace(transfer, std::move(storage));
    return transfer;
}

int APS5_VABI sceUsbdAttachKernelDriver() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceUsbdCancelTransfer() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceUsbdCheckConnected() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceUsbdClaimInterface() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceUsbdClose() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceUsbdControlTransfer() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceUsbdEventHandlingOk() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

void APS5_VABI sceUsbdFillInterruptTransfer(UsbdTransfer* transfer, void* handle, std::uint8_t endpoint, std::uint8_t* buffer, std::int32_t length, UsbdTransferCallback callback, void* userData, std::uint32_t timeout) {
    if (transfer == nullptr) throw std::runtime_error(std::string(__func__) + ": null transfer");
    if (handle != nullptr) throw std::runtime_error(ForeignHandle(__func__));
    transfer->deviceHandle = handle;
    transfer->endpoint = endpoint;
    transfer->type = TRANSFER_TYPE_INTERRUPT;
    transfer->timeout = timeout;
    transfer->buffer = buffer;
    transfer->length = length;
    transfer->userData = userData;
    transfer->callback = callback;
}

void APS5_VABI sceUsbdFreeConfigDescriptor(void* config) {
    if (config != nullptr) throw std::runtime_error(std::string(__func__) + ": the config descriptor was not returned by sceUsbdGetConfigDescriptor, which describes no devices");
}

void APS5_VABI sceUsbdFreeTransfer(UsbdTransfer* transfer) {
    if (transfer == nullptr) return;
    std::unique_ptr<std::byte[]> storage;
    {
        std::lock_guard lock(g_transferMutex);
        const auto found = g_transfers.find(transfer);
        if (found == g_transfers.end()) throw std::runtime_error(std::string(__func__) + ": transfer was not returned by sceUsbdAllocTransfer");
        storage = std::move(found->second);
        g_transfers.erase(found);
    }
    if ((transfer->flags & TRANSFER_FREE_BUFFER) != 0) ApplicationHeapFree_nid_no_patch(transfer->buffer);
}

std::int32_t APS5_VABI sceUsbdGetActiveConfigDescriptor(void* device, void** config) {
    (void)config;
    if (device == nullptr) throw std::runtime_error(std::string(__func__) + ": null device");
    throw std::runtime_error(ForeignDevice(__func__));
}

std::uint8_t APS5_VABI sceUsbdGetBusNumber(void* device) {
    if (device == nullptr) return 0;
    throw std::runtime_error(ForeignDevice(__func__));
}

std::int32_t APS5_VABI sceUsbdGetConfigDescriptor(void* device, std::uint8_t configIndex, void** config) {
    (void)configIndex;
    (void)config;
    if (device == nullptr) return SCE_USBD_ERROR_INVALID_ARG;
    throw std::runtime_error(ForeignDevice(__func__));
}

std::uint8_t APS5_VABI sceUsbdGetDeviceAddress(void* device) {
    if (device == nullptr) return 0;
    throw std::runtime_error(ForeignDevice(__func__));
}

std::int32_t APS5_VABI sceUsbdGetDeviceDescriptor(void* device, void* descriptor) {
    (void)descriptor;
    if (device == nullptr) return SCE_USBD_ERROR_INVALID_ARG;
    throw std::runtime_error(ForeignDevice(__func__));
}

int APS5_VABI sceUsbdGetStringDescriptor() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceUsbdKernelDriverActive() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::int32_t APS5_VABI sceUsbdOpen(void* device, void** handle) {
    (void)handle;
    if (device == nullptr) throw std::runtime_error(std::string(__func__) + ": null device");
    throw std::runtime_error(ForeignDevice(__func__));
}

void* APS5_VABI sceUsbdRefDevice(void* device) {
    if (device == nullptr) return nullptr;
    throw std::runtime_error(ForeignDevice(__func__));
}

int APS5_VABI sceUsbdReleaseInterface() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceUsbdResetDevice() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceUsbdSetConfiguration() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceUsbdSubmitTransfer() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

void APS5_VABI sceUsbdUnrefDevice(void* device) {
    if (device != nullptr) throw std::runtime_error(ForeignDevice(__func__));
}

}
