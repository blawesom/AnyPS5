#include <cstdint>
#include <cstdlib>

#if defined(__APPLE__)
std::uintptr_t GuestStackGuard __asm__("___stack_chk_guard_nid_postfix") = 0xDEADBEEFCAFEBABEull;
#else
std::uintptr_t __stack_chk_guard = 0xDEADBEEFCAFEBABEull;
#endif
