#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <stdexcept>
extern "C" {
int APS5_VABI seteuid_nid_postfix(std::uint32_t);
int APS5_VABI setegid_nid_postfix(std::uint32_t);
int APS5_VABI setuid_nid_postfix(std::uint32_t);
int APS5_VABI setreuid_nid_postfix(std::uint32_t, std::uint32_t);
int APS5_VABI setregid_nid_postfix(std::uint32_t, std::uint32_t);
int APS5_VABI setgroups_nid_postfix(int, const std::uint32_t*);
int APS5_VABI issetugid_nid_postfix(void);
int* APS5_VABI __error_nid_postfix();
}
using SetId = int (APS5_VABI *)(std::uint32_t);
using SetIdPair = int (APS5_VABI *)(std::uint32_t, std::uint32_t);
static constexpr int GUEST_EFAULT = 14;
static constexpr int GUEST_EINVAL = 22;
static constexpr std::uint32_t UNCHANGED = 0xFFFFFFFFu;
static void Require(bool value) { if (!value) std::abort(); }
static bool Throws(SetId function, std::uint32_t id) {
    try {
        function(id);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}
static bool ThrowsPair(SetIdPair function, std::uint32_t real, std::uint32_t effective) {
    try {
        function(real, effective);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}
static bool GroupsThrow(int count, const std::uint32_t* groups) {
    try {
        setgroups_nid_postfix(count, groups);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}
static void RequireGroupsError(int count, const std::uint32_t* groups, int error) {
    *__error_nid_postfix() = 0;
    Require(setgroups_nid_postfix(count, groups) == -1);
    Require(*__error_nid_postfix() == error);
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

    Require(setuid_nid_postfix(0) == 0);
    Require(Throws(setuid_nid_postfix, 1000));
    Require(Throws(setuid_nid_postfix, UNCHANGED));
    for (const SetIdPair function : {setreuid_nid_postfix, setregid_nid_postfix}) {
        Require(function(0, 0) == 0);
        Require(function(UNCHANGED, 0) == 0);
        Require(function(0, UNCHANGED) == 0);
        Require(function(UNCHANGED, UNCHANGED) == 0);
        Require(ThrowsPair(function, 1000, 0));
        Require(ThrowsPair(function, 0, 1000));
        Require(ThrowsPair(function, UNCHANGED, 1000));
        Require(ThrowsPair(function, 1000, UNCHANGED));
    }
    Require(*__error_nid_postfix() == 13);
    Require(issetugid_nid_postfix() == 0);

    const std::uint32_t root[] = {0};
    const std::uint32_t other[] = {1000};
    const std::uint32_t pair[] = {0, 5};
    RequireGroupsError(1025, root, GUEST_EINVAL);
    RequireGroupsError(-1, root, GUEST_EINVAL);
    RequireGroupsError(-1, nullptr, GUEST_EINVAL);
    RequireGroupsError(1, nullptr, GUEST_EFAULT);
    RequireGroupsError(1024, nullptr, GUEST_EFAULT);
    Require(GroupsThrow(1, other));
    Require(GroupsThrow(2, pair));
    Require(issetugid_nid_postfix() == 0);
    *__error_nid_postfix() = 13;
    Require(setgroups_nid_postfix(1, root) == 0);
    Require(issetugid_nid_postfix() == 1);
    Require(setgroups_nid_postfix(0, nullptr) == 0);
    Require(*__error_nid_postfix() == 13);
    Require(issetugid_nid_postfix() == 1);
}
