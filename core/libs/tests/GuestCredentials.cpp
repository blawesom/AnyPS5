#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
extern "C" {
int APS5_VABI seteuid_nid_postfix(std::uint32_t);
int APS5_VABI setegid_nid_postfix(std::uint32_t);
int APS5_VABI getlogin_r_nid_postfix(char*, int);
int* APS5_VABI __error_nid_postfix();
}
using SetId = int (APS5_VABI *)(std::uint32_t);
static void Require(bool value) { if (!value) std::abort(); }
static bool Throws(SetId function, std::uint32_t id) {
    try {
        function(id);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}
static bool LoginThrows(char* name, int length) {
    try {
        getlogin_r_nid_postfix(name, length);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}
int main() {
    *__error_nid_postfix() = 13;
    Require(seteuid_nid_postfix(0) == 0);
    Require(setegid_nid_postfix(0) == 0);
    Require(*__error_nid_postfix() == 13);
    Require(Throws(seteuid_nid_postfix, 1000));
    Require(Throws(setegid_nid_postfix, 1000));
    Require(Throws(seteuid_nid_postfix, 0xFFFFFFFFu));
    Require(Throws(setegid_nid_postfix, 0xFFFFFFFFu));
    Require(seteuid_nid_postfix(0) == 0);
    Require(setegid_nid_postfix(0) == 0);
    char name[4] = {'n', 'a', 'm', 'e'};
    Require(getlogin_r_nid_postfix(name, 0) == 34);
    Require(getlogin_r_nid_postfix(name, -1) == 34);
    Require(getlogin_r_nid_postfix(nullptr, 0) == 34);
    Require(name[0] == 'n');
    Require(*__error_nid_postfix() == 13);
    Require(LoginThrows(name, 1));
    Require(LoginThrows(name, sizeof(name)));
    Require(name[0] == 'n');
}
