#ifdef __APPLE__

#include "prx/libc/include/MainThread.hpp"
#include "prx/libc/include/Shutdown.hpp"
#include "prx/libc/include/general/AsmMacros.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <pthread.h>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>

namespace {

struct GuestTlsDescriptor {
    std::uint64_t version;
    const void* image;
    std::uint64_t fileSize;
    std::uint64_t blockSize;
    std::uint64_t alignment;
    std::uint64_t key;
};
static_assert(offsetof(GuestTlsDescriptor, key) == 0x28);

struct GuestTlsIndex {
    GuestTlsDescriptor* descriptor;
    std::uint64_t offset;
};

constexpr std::uint64_t GuestTlsVersion = 1;
constexpr std::size_t ThreadControlBytes = 0x100;
constexpr std::size_t ThreadSelfOffset = 0x00;
constexpr std::size_t ThreadAllocationOffset = 0x10;
constexpr std::size_t StackGuardOffset = 0x28;
constexpr std::size_t GuestMainStackBytes = 16 * 1024 * 1024;

std::uint64_t StackGuardValue() {
    static const std::uint64_t value = [] {
        std::uint64_t guard = 0;
        arc4random_buf(&guard, sizeof(guard));
        return guard & ~std::uint64_t{0xff};
    }();
    return value;
}

void ReleaseGuestTls(void* threadPointer) {
    if (threadPointer == nullptr) return;
    void* allocation;
    std::memcpy(&allocation, static_cast<std::uint8_t*>(threadPointer) + ThreadAllocationOffset, sizeof(allocation));
    std::free(allocation);
}

std::size_t AlignUp(const std::size_t value, const std::size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

void RequireDescriptor(const GuestTlsDescriptor* descriptor) {
    if (descriptor == nullptr || descriptor->version != GuestTlsVersion) throw std::runtime_error("Guest TLS: invalid module descriptor");
    if (descriptor->alignment == 0 || (descriptor->alignment & (descriptor->alignment - 1)) != 0 || descriptor->alignment > 8192)
        throw std::runtime_error("Guest TLS: unsupported alignment " + std::to_string(descriptor->alignment));
    if (descriptor->fileSize > descriptor->blockSize || descriptor->blockSize % descriptor->alignment != 0)
        throw std::runtime_error("Guest TLS: invalid block layout");
}

std::uint8_t* AllocateGuestTls(GuestTlsDescriptor* descriptor) {
    RequireDescriptor(descriptor);
    const auto key = static_cast<pthread_key_t>(descriptor->key);
    if (auto* existing = static_cast<std::uint8_t*>(pthread_getspecific(key)); existing != nullptr) return existing;
    const std::size_t alignment = descriptor->alignment < 16 ? 16 : static_cast<std::size_t>(descriptor->alignment);
    const std::size_t blockBytes = AlignUp(static_cast<std::size_t>(descriptor->blockSize), alignment);
    void* allocation = nullptr;
    if (posix_memalign(&allocation, alignment, blockBytes + ThreadControlBytes) != 0 || allocation == nullptr)
        throw std::runtime_error("Guest TLS: cannot allocate " + std::to_string(blockBytes + ThreadControlBytes) + " bytes");
    std::memset(allocation, 0, blockBytes + ThreadControlBytes);
    auto* threadPointer = static_cast<std::uint8_t*>(allocation) + blockBytes;
    auto* block = threadPointer - descriptor->blockSize;
    if (descriptor->fileSize != 0) {
        if (descriptor->image == nullptr) throw std::runtime_error("Guest TLS: initialization image is missing");
        std::memcpy(block, descriptor->image, static_cast<std::size_t>(descriptor->fileSize));
    }
    void* self = threadPointer;
    std::memcpy(threadPointer + ThreadSelfOffset, &self, sizeof(self));
    std::memcpy(threadPointer + ThreadAllocationOffset, &allocation, sizeof(allocation));
    const auto guard = StackGuardValue();
    std::memcpy(threadPointer + StackGuardOffset, &guard, sizeof(guard));
    if (const int error = pthread_setspecific(key, threadPointer); error != 0) {
        std::free(allocation);
        throw std::runtime_error("Guest TLS: pthread_setspecific failed with " + std::to_string(error));
    }
    return threadPointer;
}

struct GuestStart {
    void* entry;
    void* arguments;
};

void* RunGuestMain(void* context) {
    const auto start = *static_cast<GuestStart*>(context);
    delete static_cast<GuestStart*>(context);
    using GuestEntry = void (APS5_VABI*)(void*, void (*)());
    reinterpret_cast<GuestEntry>(start.entry)(start.arguments, nullptr);
    LibcExit_nid_no_patch(0);
}

[[noreturn]] void FailStart(const char* step, const int error) {
    std::fprintf(stderr, "Aps5StartGuest: %s failed: %s\n", step, std::strerror(error));
    LibcTerminate_nid_no_patch(1);
}

}

extern "C" __attribute__((visibility("hidden"))) std::uint8_t* Aps5GuestTlsAllocateSlow(GuestTlsDescriptor* descriptor) {
    return AllocateGuestTls(descriptor);
}

asm(
    ".text\n"
    ".p2align 4\n"
    APS5_ASM_FUNCTION("Aps5GuestTlsAllocate_nid_no_patch")
    "pushq %rbp\n"
    "movq %rsp, %rbp\n"
    "pushq %rdi\n"
    "pushq %rsi\n"
    "pushq %rdx\n"
    "pushq %rcx\n"
    "pushq %r8\n"
    "pushq %r9\n"
    "pushq %r10\n"
    "pushq %r11\n"
    "movq %rax, %rdi\n"
    "andq $-32, %rsp\n"
    "subq $512, %rsp\n"
    "vmovdqa %ymm0, 0(%rsp)\n"
    "vmovdqa %ymm1, 32(%rsp)\n"
    "vmovdqa %ymm2, 64(%rsp)\n"
    "vmovdqa %ymm3, 96(%rsp)\n"
    "vmovdqa %ymm4, 128(%rsp)\n"
    "vmovdqa %ymm5, 160(%rsp)\n"
    "vmovdqa %ymm6, 192(%rsp)\n"
    "vmovdqa %ymm7, 224(%rsp)\n"
    "vmovdqa %ymm8, 256(%rsp)\n"
    "vmovdqa %ymm9, 288(%rsp)\n"
    "vmovdqa %ymm10, 320(%rsp)\n"
    "vmovdqa %ymm11, 352(%rsp)\n"
    "vmovdqa %ymm12, 384(%rsp)\n"
    "vmovdqa %ymm13, 416(%rsp)\n"
    "vmovdqa %ymm14, 448(%rsp)\n"
    "vmovdqa %ymm15, 480(%rsp)\n"
    "call " APS5_ASM_SYMBOL("Aps5GuestTlsAllocateSlow") "\n"
    "vmovdqa 0(%rsp), %ymm0\n"
    "vmovdqa 32(%rsp), %ymm1\n"
    "vmovdqa 64(%rsp), %ymm2\n"
    "vmovdqa 96(%rsp), %ymm3\n"
    "vmovdqa 128(%rsp), %ymm4\n"
    "vmovdqa 160(%rsp), %ymm5\n"
    "vmovdqa 192(%rsp), %ymm6\n"
    "vmovdqa 224(%rsp), %ymm7\n"
    "vmovdqa 256(%rsp), %ymm8\n"
    "vmovdqa 288(%rsp), %ymm9\n"
    "vmovdqa 320(%rsp), %ymm10\n"
    "vmovdqa 352(%rsp), %ymm11\n"
    "vmovdqa 384(%rsp), %ymm12\n"
    "vmovdqa 416(%rsp), %ymm13\n"
    "vmovdqa 448(%rsp), %ymm14\n"
    "vmovdqa 480(%rsp), %ymm15\n"
    "leaq -64(%rbp), %rsp\n"
    "popq %r11\n"
    "popq %r10\n"
    "popq %r9\n"
    "popq %r8\n"
    "popq %rcx\n"
    "popq %rdx\n"
    "popq %rsi\n"
    "popq %rdi\n"
    "popq %rbp\n"
    "ret\n"
);

extern "C" void Aps5GuestTlsRegister_nid_no_patch(GuestTlsDescriptor* descriptor) {
    RequireDescriptor(descriptor);
    pthread_key_t key;
    if (const int error = pthread_key_create(&key, ReleaseGuestTls); error != 0)
        throw std::runtime_error("Guest TLS: pthread_key_create failed with " + std::to_string(error));
    descriptor->key = static_cast<std::uint64_t>(key);
}

extern "C" void* APS5_VABI Aps5GuestTlsGetAddr_nid_no_patch(const GuestTlsIndex* index) {
    if (index == nullptr) throw std::runtime_error("Guest TLS: null TLS index");
    auto* threadPointer = AllocateGuestTls(index->descriptor);
    if (index->offset >= index->descriptor->blockSize) throw std::runtime_error("Guest TLS: offset " + std::to_string(index->offset) + " is outside the TLS block");
    return threadPointer - index->descriptor->blockSize + index->offset;
}

extern "C" [[noreturn]] void Aps5StartGuest_nid_no_patch(void* entry, void* arguments) {
    if (entry == nullptr || arguments == nullptr) {
        std::fprintf(stderr, "Aps5StartGuest: missing entry point or arguments\n");
        LibcTerminate_nid_no_patch(1);
    }
    std::signal(SIGPIPE, SIG_IGN);
    pthread_attr_t attributes;
    if (const int error = pthread_attr_init(&attributes); error != 0) FailStart("pthread_attr_init", error);
    if (const int error = pthread_attr_setstacksize(&attributes, GuestMainStackBytes); error != 0) FailStart("pthread_attr_setstacksize", error);
    if (const int error = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED); error != 0) FailStart("pthread_attr_setdetachstate", error);
    pthread_t thread;
    auto* start = new GuestStart{entry, arguments};
    if (const int error = pthread_create(&thread, &attributes, RunGuestMain, start); error != 0) {
        delete start;
        FailStart("pthread_create", error);
    }
    pthread_attr_destroy(&attributes);
    LibcServeMainThread_nid_postfix();
}

#endif
