#include "prx/libc/include/general/VabiMacros.hpp"

#include <cstdlib>

extern "C" {
int APS5_VABI sceCommonDialogInitialize(void);
bool APS5_VABI sceCommonDialogIsUsed(void);
}

namespace {

constexpr int kErrAlreadyInitialized = static_cast<int>(0x80B80004);

void Require(bool value) { if (!value) std::abort(); }

}

int main() {
    Require(!sceCommonDialogIsUsed());
    Require(sceCommonDialogInitialize() == 0);
    Require(sceCommonDialogInitialize() == kErrAlreadyInitialized);
    Require(!sceCommonDialogIsUsed());
}
