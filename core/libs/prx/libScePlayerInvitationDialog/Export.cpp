#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI scePlayerInvitationDialogTerminate(void) {
 return 0;
}

int APS5_VABI scePlayerInvitationDialogUpdateStatus(void) {
 return 0;
}

int APS5_VABI scePlayerInvitationDialogInitialize(void) {
 NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI scePlayerInvitationDialogOpen(const void* param) {
 (void)param;
 NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI scePlayerInvitationDialogGetStatus(void) {
 NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI scePlayerInvitationDialogGetResult(void* result) {
 (void)result;
 NotImplemented_nid_no_patch(__func__);
}

int APS5_VABI scePlayerInvitationDialogClose(void) {
 NotImplemented_nid_no_patch(__func__);
}

}
