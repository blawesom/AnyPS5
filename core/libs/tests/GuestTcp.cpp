#include "prx/libc/include/General.hpp"
#include "prx/libkernel/File/include/FileFlags.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

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

extern "C" {
int APS5_VABI socket_nid_postfix(int, int, int);
int APS5_VABI bind_nid_postfix(int, const void*, std::uint32_t);
int APS5_VABI listen_nid_postfix(int, int);
int APS5_VABI getsockname_nid_postfix(int, void*, std::uint32_t*);
int APS5_VABI connect_nid_postfix(int, const void*, std::uint32_t);
int APS5_VABI accept_nid_postfix(int, void*, std::uint32_t*);
std::int64_t APS5_VABI send_nid_postfix(int, const void*, std::uint64_t, int);
std::int64_t APS5_VABI recv_nid_postfix(int, void*, std::uint64_t, int);
int APS5_VABI getpeername_nid_postfix(int, void*, std::uint32_t*);
int APS5_VABI shutdown_nid_postfix(int, int);
int APS5_VABI close_nid_postfix(int);
int APS5_VABI fcntl_nid_postfix(int, int, ...);
int APS5_VABI setsockopt_nid_postfix(int, int, int, const void*, std::uint32_t);
int APS5_VABI sceKernelOpen(const char*, int, std::uint16_t);
std::int64_t APS5_VABI lseek_nid_postfix(int, std::int64_t, int);
int APS5_VABI sendfile_nid_postfix(int, int, std::int64_t, std::uint64_t, const GuestHeaderTrailer*, std::int64_t*, int);
int* APS5_VABI __error_nid_postfix();
}

static void Require(bool condition) {
    if (!condition) std::abort();
}

static std::string Pattern(std::size_t length) {
    std::string bytes(length, '\0');
    for (std::size_t i = 0; i < length; ++i) bytes[i] = static_cast<char>((i * 131 + i / 251) & 0xff);
    return bytes;
}

static std::string Receive(int socket, std::size_t length) {
    std::string bytes(length, '\0');
    for (std::size_t done = 0; done < length;) {
        const auto received = recv_nid_postfix(socket, bytes.data() + done, length - done, 0);
        Require(received > 0);
        done += static_cast<std::size_t>(received);
    }
    return bytes;
}

static void RequireSendFile(int file, int client, int accepted, std::int64_t offset, std::uint64_t nbytes,
    const GuestHeaderTrailer* hdtr, int flags, const std::string& expected) {
    std::int64_t sent = -1;
    Require(sendfile_nid_postfix(file, client, offset, nbytes, hdtr, &sent, flags) == 0);
    Require(sent == static_cast<std::int64_t>(expected.size()));
    Require(Receive(accepted, expected.size()) == expected);
}

static void RequireSendFileFailure(int file, int socket, std::int64_t offset, int error, std::int64_t reported) {
    std::int64_t sent = 99;
    *__error_nid_postfix() = 0;
    Require(sendfile_nid_postfix(file, socket, offset, 0, nullptr, &sent, 0) == -1);
    Require(*__error_nid_postfix() == error && sent == reported);
}

static void SendFileCases(int client, int accepted) {
    const auto root = std::filesystem::temp_directory_path() /
        ("anyps5-sendfile-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Require(std::filesystem::create_directory(root));
    const auto content = Pattern(10000);
    const auto large = Pattern(16 * 1024 * 1024);
    std::ofstream(root / "file.bin", std::ios::binary) << content;
    std::ofstream(root / "large.bin", std::ios::binary) << large;
    AddPathAlias_nid_no_patch("/anyps5-sendfile-test", root.string().c_str());
    const int file = sceKernelOpen("/anyps5-sendfile-test/file.bin", SCE_KERNEL_O_RDONLY, 0);
    Require(file >= 0);

    RequireSendFile(file, client, accepted, 0, 0, nullptr, 0, content);
    RequireSendFile(file, client, accepted, 4000, 0, nullptr, 0, content.substr(4000));
    RequireSendFile(file, client, accepted, 2500, 1000, nullptr, 0, content.substr(2500, 1000));
    RequireSendFile(file, client, accepted, 9500, 2000, nullptr, 0, content.substr(9500));
    RequireSendFile(file, client, accepted, 10000, 0, nullptr, 0, "");
    RequireSendFile(file, client, accepted, 20000, 5, nullptr, 0, "");
    const GuestIovec headers[] = {{"HEAD", 4}, {nullptr, 0}, {"ER:", 3}};
    const GuestIovec trailers[] = {{":TRAIL", 6}};
    const GuestHeaderTrailer hdtr{headers, 3, trailers, 1};
    RequireSendFile(file, client, accepted, 100, 50, &hdtr, 0x1 | 0x4 | 0x10, "HEADER:" + content.substr(100, 50) + ":TRAIL");
    const GuestHeaderTrailer headersOnly{headers, 3, nullptr, 7};
    RequireSendFile(file, client, accepted, 9990, 0, &headersOnly, 0x10000, "HEADER:" + content.substr(9990));
    Require(sendfile_nid_postfix(file, client, 0, 10, nullptr, nullptr, 0) == 0);
    Require(Receive(accepted, 10) == content.substr(0, 10));
    Require(lseek_nid_postfix(file, 0, 1) == 0);

    const GuestHeaderTrailer tooMany{headers, 1025, nullptr, 0};
    std::int64_t sent = 99;
    Require(sendfile_nid_postfix(file, client, 0, 0, &tooMany, &sent, 0) == -1 && *__error_nid_postfix() == 22 && sent == 99);
    RequireSendFileFailure(file, client, -1, 22, 99);
    const int other = sceKernelOpen("/anyps5-sendfile-test/file.bin", SCE_KERNEL_O_RDONLY, 0);
    Require(other >= 0);
    RequireSendFileFailure(file, other, 0, 38, 0);
    const int writeOnly = sceKernelOpen("/anyps5-sendfile-test/file.bin", SCE_KERNEL_O_WRONLY, 0);
    Require(writeOnly >= 0);
    RequireSendFileFailure(writeOnly, client, 0, 9, 99);
    Require(close_nid_postfix(writeOnly) == 0);
    Require(close_nid_postfix(other) == 0);
    RequireSendFileFailure(other, client, 0, 9, 99);
    RequireSendFileFailure(client, accepted, 0, 22, 0);
    RequireSendFileFailure(file, 0x7ffffff0, 0, 9, 0);
    const int datagram = socket_nid_postfix(2, 2, 0);
    Require(datagram >= 0);
    RequireSendFileFailure(file, datagram, 0, 22, 0);
    Require(close_nid_postfix(datagram) == 0);
    const int unconnected = socket_nid_postfix(2, 1, 0);
    Require(unconnected >= 0);
    RequireSendFileFailure(file, unconnected, 0, 57, 0);
    Require(close_nid_postfix(unconnected) == 0);

    const int big = sceKernelOpen("/anyps5-sendfile-test/large.bin", SCE_KERNEL_O_RDONLY, 0);
    Require(big >= 0);
    const int sendBuffer = 16384;
    Require(setsockopt_nid_postfix(client, 0xffff, 0x1001, &sendBuffer, sizeof(sendBuffer)) == 0);
    Require(fcntl_nid_postfix(client, 4, 4) == 0);
    const GuestIovec header[] = {{"H", 1}};
    const GuestHeaderTrailer headerAndTrailer{header, 1, trailers, 1};
    sent = -1;
    *__error_nid_postfix() = 0;
    Require(sendfile_nid_postfix(big, client, 0, 0, &headerAndTrailer, &sent, 0) == -1 && *__error_nid_postfix() == 35);
    Require(sent > 1 && sent < static_cast<std::int64_t>(large.size()));
    Require(Receive(accepted, static_cast<std::size_t>(sent)) == "H" + large.substr(0, static_cast<std::size_t>(sent) - 1));
    Require(fcntl_nid_postfix(client, 4, 0) == 0);
    Require(close_nid_postfix(big) == 0);
    Require(close_nid_postfix(file) == 0);
    std::filesystem::remove_all(root);
}

int main() {
    const int listener = socket_nid_postfix(2, 1, 6);
    Require(listener >= 0);
    std::array<std::uint8_t, 16> address{16, 2, 0, 0, 127, 0, 0, 1};
    Require(bind_nid_postfix(listener, address.data(), address.size()) == 0);
    Require(bind_nid_postfix(listener, address.data(), address.size()) == -1 && *__error_nid_postfix() == 22);
    Require(listen_nid_postfix(listener, 4) == 0);
    std::uint32_t address_size = address.size();
    Require(getsockname_nid_postfix(listener, address.data(), &address_size) == 0);
    Require(address_size == 16 && (address[2] != 0 || address[3] != 0));

    const int client = socket_nid_postfix(2, 1, 0);
    Require(client >= 0);
    Require(accept_nid_postfix(client, nullptr, nullptr) == -1 && *__error_nid_postfix() == 22);
    Require(connect_nid_postfix(client, address.data(), address.size()) == 0);
    std::array<std::uint8_t, 16> peer{};
    address_size = peer.size();
    const int accepted = accept_nid_postfix(listener, peer.data(), &address_size);
    Require(accepted >= 0 && address_size == 16 && peer[1] == 2);

    std::array<std::uint8_t, 16> connected_peer{};
    address_size = connected_peer.size();
    Require(getpeername_nid_postfix(accepted, connected_peer.data(), &address_size) == 0);
    Require(address_size == 16 && connected_peer[1] == 2);
    SendFileCases(client, accepted);

    const char request[] = "guest TCP loopback";
    char received[sizeof(request)]{};
    Require(send_nid_postfix(client, request, sizeof(request), 0x1) == -1 && *__error_nid_postfix() == 45);
    Require(send_nid_postfix(client, request, sizeof(request), 0x20000) == sizeof(request));
    Require(recv_nid_postfix(accepted, received, sizeof(received), 0) == sizeof(received));
    Require(std::strcmp(request, received) == 0);
    Require(shutdown_nid_postfix(client, 1) == 0);
    Require(send_nid_postfix(client, request, sizeof(request), 0x20000) == -1 && *__error_nid_postfix() == 32);

    Require(close_nid_postfix(accepted) == 0);
#ifndef _WIN32
    std::int64_t sent = 0;
    for (int i = 0; i < 100 && sent >= 0; ++i) sent = send_nid_postfix(client, request, sizeof(request), 0x20000);
    Require(sent == -1 && *__error_nid_postfix() == 32);
#endif
    Require(close_nid_postfix(client) == 0);
    Require(close_nid_postfix(listener) == 0);
    Require(close_nid_postfix(listener) == -1 && *__error_nid_postfix() == 9);
}