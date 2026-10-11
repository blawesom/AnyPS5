#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>
struct PollDescriptor {
    int descriptor;
    short events;
    short revents;
};
extern "C" {
int APS5_VABI socket_nid_postfix(int, int, int);
int APS5_VABI bind_nid_postfix(int, const void*, std::uint32_t);
int APS5_VABI listen_nid_postfix(int, int);
int APS5_VABI accept_nid_postfix(int, void*, std::uint32_t*);
int APS5_VABI connect_nid_postfix(int, const void*, std::uint32_t);
int APS5_VABI getsockname_nid_postfix(int, void*, std::uint32_t*);
std::int64_t APS5_VABI send_nid_postfix(int, const void*, std::uint64_t, int);
std::int64_t APS5_VABI recv_nid_postfix(int, void*, std::uint64_t, int);
int APS5_VABI close_nid_postfix(int);
int APS5_VABI open_nid_postfix(const char*, int, int);
int APS5_VABI poll_nid_postfix(PollDescriptor*, std::uint32_t, int);
int* APS5_VABI __error_nid_postfix();
}
static void Require(bool value) { if (!value) std::abort(); }
constexpr short In = 0x1;
constexpr short Out = 0x4;
constexpr short Hup = 0x10;
constexpr short Invalid = 0x20;
constexpr int CreateReadWrite = 0x602;
template <typename TCall> static bool Throws(TCall call) {
    try {
        call();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}
static void PollsFilesNextToSockets() {
    const auto path = std::filesystem::path("anyps5-poll-test-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".bin");
    const int file = open_nid_postfix(path.string().c_str(), CreateReadWrite, 0644);
    Require(file >= 0);
    PollDescriptor regular{file, In | Out, -1};
    Require(poll_nid_postfix(&regular, 1, 0) == 1 && regular.revents == (In | Out));
    const int datagram = socket_nid_postfix(2, 2, 0);
    Require(datagram >= 0);
    std::array<unsigned char, 16> address{16, 2, 0, 0, 127, 0, 0, 1};
    Require(bind_nid_postfix(datagram, address.data(), address.size()) == 0);
    PollDescriptor mixed[2]{{file, In, -1}, {datagram, In, -1}};
    auto start = std::chrono::steady_clock::now();
    Require(poll_nid_postfix(mixed, 2, -1) == 1 && mixed[0].revents == In && mixed[1].revents == 0);
    Require(std::chrono::steady_clock::now() - start < std::chrono::milliseconds(100));
    PollDescriptor idle{datagram, In, -1};
    start = std::chrono::steady_clock::now();
    Require(poll_nid_postfix(&idle, 1, 200) == 0 && idle.revents == 0);
    const auto waited = std::chrono::steady_clock::now() - start;
    Require(waited >= std::chrono::milliseconds(200) && waited < std::chrono::seconds(2));
    Require(close_nid_postfix(file) == 0);
    PollDescriptor closed[2]{{file, In | Out, -1}, {datagram, In, -1}};
    Require(poll_nid_postfix(closed, 2, -1) == 1 && closed[0].revents == Invalid && closed[1].revents == 0);
    Require(Throws([] { poll_nid_postfix(nullptr, 1025, 0); }));
    Require(close_nid_postfix(datagram) == 0);
    Require(std::filesystem::remove(path));
}
int main() {
    PollsFilesNextToSockets();
    const int listener = socket_nid_postfix(2, 1, 0);
    Require(listener >= 0);
    std::array<unsigned char, 16> address{16, 2, 0, 0, 127, 0, 0, 1};
    Require(bind_nid_postfix(listener, address.data(), address.size()) == 0);
    std::uint32_t size = address.size();
    Require(getsockname_nid_postfix(listener, address.data(), &size) == 0);
    Require(listen_nid_postfix(listener, 4) == 0);
    PollDescriptor single{listener, In, -1};
    Require(poll_nid_postfix(&single, 1, 0) == 0 && single.revents == 0);
    const auto start = std::chrono::steady_clock::now();
    Require(poll_nid_postfix(&single, 1, 50) == 0);
    Require(std::chrono::steady_clock::now() - start >= std::chrono::milliseconds(40));
    const int client = socket_nid_postfix(2, 1, 0);
    Require(client >= 0 && connect_nid_postfix(client, address.data(), address.size()) == 0);
    Require(poll_nid_postfix(&single, 1, 1000) == 1 && single.revents == In);
    const int server = accept_nid_postfix(listener, nullptr, nullptr);
    Require(server >= 0);
    PollDescriptor pair[3]{{server, In, 0}, {-1, In, 7}, {client, Out, 0}};
    Require(poll_nid_postfix(pair, 3, 1000) == 1);
    Require(pair[0].revents == 0 && pair[1].revents == 0 && pair[2].revents == Out);
    const char message[] = "poll";
    Require(send_nid_postfix(client, message, sizeof(message), 0) == sizeof(message));
    pair[2].events = In;
    Require(poll_nid_postfix(pair, 3, 1000) == 1 && pair[0].revents == In && pair[2].revents == 0);
    char buffer[8]{};
    Require(recv_nid_postfix(server, buffer, sizeof(buffer), 0) == sizeof(message));
    Require(close_nid_postfix(client) == 0);
    Require(poll_nid_postfix(pair, 1, 1000) == 1 && (pair[0].revents & (In | Hup)) != 0);
    Require(recv_nid_postfix(server, buffer, sizeof(buffer), 0) == 0);
    PollDescriptor closed[2]{{client, In, 0}, {listener, In, 0}};
    Require(poll_nid_postfix(closed, 2, -1) == 1 && closed[0].revents == Invalid && closed[1].revents == 0);
    Require(poll_nid_postfix(nullptr, 0, 10) == 0);
    Require(poll_nid_postfix(nullptr, 1, 0) == -1 && *__error_nid_postfix() == 14);
    Require(poll_nid_postfix(&single, 1, -2) == -1 && *__error_nid_postfix() == 22);
    Require(close_nid_postfix(server) == 0);
    Require(close_nid_postfix(listener) == 0);
}
