#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern "C" {
int APS5_VABI socket_nid_postfix(int, int, int);
int APS5_VABI connect_nid_postfix(int, const void*, std::uint32_t);
int APS5_VABI bind_nid_postfix(int, const void*, std::uint32_t);
int APS5_VABI getsockname_nid_postfix(int, void*, std::uint32_t*);
int APS5_VABI getpeername_nid_postfix(int, void*, std::uint32_t*);
std::int64_t APS5_VABI sendto_nid_postfix(int, const void*, std::uint64_t, int, const void*, std::uint32_t);
int APS5_VABI close_nid_postfix(int);
int* APS5_VABI __error_nid_postfix();
}
struct Iovec {
    void* base;
    std::uint64_t length;
};
struct Msghdr {
    void* name;
    std::uint32_t nameLength;
    Iovec* iov;
    int iovLength;
    void* control;
    std::uint32_t controlLength;
    int flags;
};
extern "C" std::int64_t APS5_VABI sendmsg_nid_postfix(int, const Msghdr*, int);

static void Require(bool value, const char* message) {
    if (!value) {
        std::fprintf(stderr, "%s (errno %d)\n", message, *__error_nid_postfix());
        std::exit(1);
    }
}

static void Check(int family, int type) {
    const std::uint32_t size = family == 2 ? 16 : 28;
    const int probe = socket_nid_postfix(family, type, 0);
    if (probe < 0 && family == 28 && *__error_nid_postfix() == 47) return;
    Require(probe >= 0, "socket");
    std::array<unsigned char, 28> address{};
    address[0] = static_cast<unsigned char>(size);
    address[1] = static_cast<unsigned char>(family);
    if (family == 2) { address[4] = 127; address[7] = 1; }
    else address[23] = 1;
    std::uint32_t length = address.size();
    std::array<unsigned char, 28> name{};
    Require(connect_nid_postfix(probe, address.data(), size) == -1 && *__error_nid_postfix() == 49,
        "connect to port 0 must fail with EADDRNOTAVAIL (49)");
    Require(getpeername_nid_postfix(probe, name.data(), &length) == -1 && *__error_nid_postfix() == 57, "still unconnected");
    if (type == 2) {
        Require(sendto_nid_postfix(probe, "x", 1, 0, address.data(), size) == -1 && *__error_nid_postfix() == 49,
            "sendto port 0 must fail with EADDRNOTAVAIL (49)");
        Iovec vector{const_cast<char*>("x"), 1};
        Msghdr message{address.data(), size, &vector, 1, nullptr, 0, 0};
        Require(sendmsg_nid_postfix(probe, &message, 0) == -1 && *__error_nid_postfix() == 49,
            "sendmsg to port 0 must fail with EADDRNOTAVAIL (49)");
    }
    Require(close_nid_postfix(probe) == 0, "close");
}

int main() {
    Check(2, 2);
    Check(2, 1);
    Check(28, 2);
    Check(28, 1);
    return 0;
}
