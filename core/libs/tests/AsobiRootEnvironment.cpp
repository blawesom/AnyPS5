#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdlib>
#include <cstring>
extern "C" {
char* APS5_VABI getenv_nid_postfix(const char*);
int APS5_VABI setenv_nid_postfix(const char*, const char*, int);
}
static void Require(bool value) { if (!value) std::abort(); }
int main() {
    Require(getenv_nid_postfix("ASOBI_ROOT") != nullptr);
    Require(std::strcmp(getenv_nid_postfix("ASOBI_ROOT"), "app0") == 0);
    Require(setenv_nid_postfix("ASOBI_ROOT", "override", 1) == 0);
    Require(std::strcmp(getenv_nid_postfix("ASOBI_ROOT"), "override") == 0);
    Require(getenv_nid_postfix("ASOBI_ROOT_NONEXISTENT") == nullptr);
}
