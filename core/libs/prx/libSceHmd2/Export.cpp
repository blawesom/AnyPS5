#include "Hmd2.hpp"
#include "prx/libc/include/general/ExportMacros.hpp"
#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libSceHmd/Hmd.hpp"
#include "prx/libSceUserService/UserService.hpp"
#include "SceTypes.hpp"

#include <cstring>
#include <mutex>
#include <stdexcept>

namespace {

constexpr std::int32_t DeviceHandle = 0x0e010000;
constexpr Hmd2::FieldOfView DefaultFieldOfView{1.84177089f, 0.947190642f, 1.32898104f, 1.32898104f};

std::mutex stateMutex;
bool initialized = false;
std::int32_t openHandle = 0;

std::int32_t BeforeInitialize(std::int32_t answer, const char* name) {
    std::lock_guard lock(stateMutex);
    if (initialized) NotImplemented_nid_no_patch(name);
    return answer;
}

std::uint32_t CompiledSdkVersion() {
    static const auto* processParameters = static_cast<const std::byte*>(ApplicationProcessParameters_nid_no_patch());
    std::uint32_t version;
    std::memcpy(&version, processParameters + 0x10, sizeof(version));
    return version;
}

std::int32_t GetDeviceInformation(Hmd2::DeviceInformation* info) {
    if (!info) return Hmd2::ParameterNull;
    *info = {};
    if (!initialized) return Hmd2::NotInitialized;
    info->status = Hmd2::DeviceStatus::NotDetected;
    if (CompiledSdkVersion() < Hmd2::UserIdSdkVersion) info->userId = -1;
    info->panelWidth = 4000;
    info->panelHeight = 2040;
    info->latency90Hz = 5555;
    info->latency120Hz = 4160;
    info->virtualImageDistance = 2.0f;
    return 0;
}

}

extern "C" {

int APS5_VABI sceHmd2ReprojectionGetStatus() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

Hmd2::SizeAlign APS5_VABI sceHmd2ReprojectionQuerySeeThroughBufferSizeAlign() {
    return {0x1106000, 0x100};
}

std::int32_t APS5_VABI sceHmd2Initialize(const void* param) {
    (void)param;
    return Hmd::UnsupportedFeature;
}

std::int32_t APS5_VABI sceHmd2GetFieldOfView(std::int32_t handle, Hmd2::FieldOfView* fieldOfView) {
    std::lock_guard lock(stateMutex);
    if (!fieldOfView) return Hmd2::ParameterNull;
    if (handle == 0 || handle != openHandle) return Hmd2::InvalidHandle;
    if (!initialized) return Hmd2::NotInitialized;
    *fieldOfView = DefaultFieldOfView;
    return 0;
}

std::int32_t APS5_VABI sceHmd2GetDeviceInformationByHandle(std::int32_t handle, Hmd2::DeviceInformation* info) {
    std::lock_guard lock(stateMutex);
    if (handle != openHandle) return Hmd2::InvalidHandle;
    return GetDeviceInformation(info);
}

std::int32_t APS5_VABI sceHmd2Terminate() {
    std::lock_guard lock(stateMutex);
    if (!initialized) return Hmd2::NotInitialized;
    initialized = false;
    return 0;
}

std::int32_t APS5_VABI sceHmd2Close(std::int32_t handle) {
    std::lock_guard lock(stateMutex);
    if (!initialized) return Hmd2::NotInitialized;
    if (handle != openHandle) return Hmd2::InvalidHandle;
    openHandle = 0;
    return 0;
}

int APS5_VABI sceHmd2GazeGetResult() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2GazeGetResultForFoveatedRendering() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::int32_t APS5_VABI sceHmd2GetDeviceInformation(Hmd2::DeviceInformation* info) {
    std::lock_guard lock(stateMutex);
    return GetDeviceInformation(info);
}

std::int32_t APS5_VABI sceHmd2GetFieldOfViewWithoutHandle(Hmd2::FieldOfView* fieldOfView) {
    std::lock_guard lock(stateMutex);
    if (!fieldOfView) return Hmd2::ParameterNull;
    if (!initialized) return Hmd2::NotInitialized;
    *fieldOfView = DefaultFieldOfView;
    return 0;
}

std::int32_t APS5_VABI sceHmd2Open(std::int32_t userId, std::int32_t type, std::int32_t index, const void* param) {
    std::lock_guard lock(stateMutex);
    if (!initialized) return Hmd2::NotInitialized;
    if (type != 0 || index != 0 || param) return Hmd2::ParameterInvalid;
    if (userId != USER_SERVICE_INITIAL_USER_ID) {
        if (CompiledSdkVersion() >= Hmd2::UserIdSdkVersion) return Hmd2::ParameterInvalid;
        throw std::runtime_error("sceHmd2Open: user is not the initial user");
    }
    if (openHandle != 0) return Hmd2::AlreadyOpened;
    openHandle = DeviceHandle;
    return DeviceHandle;
}

int APS5_VABI sceHmd2ReprojectionBeginFrame() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionDisableMirroring() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionDisableVrMode() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionEnableMirroring() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionEnableVrMode() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

Hmd2::SizeAlign APS5_VABI sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign() {
    return {0x5FA0000, 0x10000};
}

Hmd2::SizeAlign APS5_VABI sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign() {
    return {0x4000, 0x100};
}

int APS5_VABI sceHmd2ReprojectionGetPredictedDisplayTime() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionInitialize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionQueryBufferSizeAlign() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

Hmd2::SizeAlign APS5_VABI sceHmd2ReprojectionQueryDisplayBufferSizeAlign() {
    return {0x6000000, 0x10000};
}

int APS5_VABI sceHmd2ReprojectionSetAllowPositionalReprojection() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionSetMirroringOption() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionSetParam() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionSetParamWithBuffer() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionSetRenderConfig() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionSetTiming() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionTerminate() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

std::int32_t APS5_VABI sceHmd2SetVibration(std::int32_t handle, std::uint32_t level) {
    (void)level;
    std::lock_guard lock(stateMutex);
    if (handle != openHandle) return Hmd2::InvalidHandle;
    return 0;
}

int APS5_VABI sceHmd2GazeGetCalibrationDataMaxSize() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2GazeGetDeviceStatus() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2GazeGetUserCalibrationStatus() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2GazeInsertRawDataWithHandle() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionSeeThroughDraw() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionSeeThroughIC() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionVideoOutEq() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionWaitShader() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2SeeThroughCallbackGetPoseDataFunc() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2GazeObjectSelectionGetSelectedObject() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2GazeObjectSelectionQueryMemory() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionClear() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionSet() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionSetEventGpuEnd() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2ReprojectionSetEventGpuStart() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2RpeorjectionSetParam() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2SetFieldOfView() {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}


std::int32_t APS5_VABI Hmd2BeforeInitializeZero() {
    return BeforeInitialize(0, __func__);
}
APS5_EXPORT("vDgpdS2N9Rw", Hmd2BeforeInitializeZero);
APS5_EXPORT("Ri35gKTZpvE", Hmd2BeforeInitializeZero);
APS5_EXPORT("9oyW8puiKeI", Hmd2BeforeInitializeZero);
APS5_EXPORT("1HZNE6jhBnQ", Hmd2BeforeInitializeZero);
APS5_EXPORT("cdwAKBZLXjs", Hmd2BeforeInitializeZero);
APS5_EXPORT("Yub1MF58jfY", Hmd2BeforeInitializeZero);
APS5_EXPORT("JYXAuQFnPyw", Hmd2BeforeInitializeZero);
APS5_EXPORT("VuPKqCu3wzw", Hmd2BeforeInitializeZero);
APS5_EXPORT("GSThagVyW5U", Hmd2BeforeInitializeZero);
APS5_EXPORT("J4qE5N937MY", Hmd2BeforeInitializeZero);
APS5_EXPORT("9KnNj-dyASg", Hmd2BeforeInitializeZero);
APS5_EXPORT("Qta2bkstbFk", Hmd2BeforeInitializeZero);
APS5_EXPORT("5ZcObnDRBXE", Hmd2BeforeInitializeZero);
APS5_EXPORT("AvPNn0qx4WQ", Hmd2BeforeInitializeZero);
APS5_EXPORT("jp0apqNGgi8", Hmd2BeforeInitializeZero);
APS5_EXPORT("-kZT7X7OMg0", Hmd2BeforeInitializeZero);
APS5_EXPORT("1ewLtng-SVk", Hmd2BeforeInitializeZero);
APS5_EXPORT("B0b8DeIUGes", Hmd2BeforeInitializeZero);
APS5_EXPORT("HclLg4Oj5yw", Hmd2BeforeInitializeZero);
APS5_EXPORT("VbEvrQXsZyY", Hmd2BeforeInitializeZero);
APS5_EXPORT("bB6MKZi5UCQ", Hmd2BeforeInitializeZero);
APS5_EXPORT("OXQ1EDf02XU", Hmd2BeforeInitializeZero);
APS5_EXPORT("IAxfMTvFm0M", Hmd2BeforeInitializeZero);
APS5_EXPORT("qTJtAtaLeno", Hmd2BeforeInitializeZero);
APS5_EXPORT("j7Q16UX6l34", Hmd2BeforeInitializeZero);
APS5_EXPORT("pmmNM06rLqU", Hmd2BeforeInitializeZero);
APS5_EXPORT("zdQfyDK9Ods", Hmd2BeforeInitializeZero);
APS5_EXPORT("aPRwp9GnljM", Hmd2BeforeInitializeZero);
APS5_EXPORT("MjY1gBSaed0", Hmd2BeforeInitializeZero);
APS5_EXPORT("evAMjJYHqW4", Hmd2BeforeInitializeZero);
APS5_EXPORT("r93FLEZ3BBk", Hmd2BeforeInitializeZero);
APS5_EXPORT("IdYURdEdNFo", Hmd2BeforeInitializeZero);
APS5_EXPORT("CVcL2Cu2AEI", Hmd2BeforeInitializeZero);
APS5_EXPORT("NmKIfVxr2s0", Hmd2BeforeInitializeZero);
APS5_EXPORT("KeDLkkp62Gg", Hmd2BeforeInitializeZero);
APS5_EXPORT("+aadu6-ra+A", Hmd2BeforeInitializeZero);
APS5_EXPORT("FgnTiwwGSzk", Hmd2BeforeInitializeZero);
APS5_EXPORT("PWAUwOTHbbo", Hmd2BeforeInitializeZero);
APS5_EXPORT("WzaEApHnsP8", Hmd2BeforeInitializeZero);
APS5_EXPORT("dXzQI8QVY7Y", Hmd2BeforeInitializeZero);
APS5_EXPORT("cCvFKGqRh1k", Hmd2BeforeInitializeZero);
APS5_EXPORT("uj4-2VEAFPU", Hmd2BeforeInitializeZero);
APS5_EXPORT("sIaGStI-iLM", Hmd2BeforeInitializeZero);
APS5_EXPORT("JzwpZOemDO8", Hmd2BeforeInitializeZero);
APS5_EXPORT("IsC4R4bZFVU", Hmd2BeforeInitializeZero);

std::int32_t APS5_VABI Hmd2BeforeInitializeNotInitialized() {
    return BeforeInitialize(Hmd2::NotInitialized, __func__);
}
APS5_EXPORT("sXaPEPY-fLA", Hmd2BeforeInitializeNotInitialized);
APS5_EXPORT("3F5inHhKzXg", Hmd2BeforeInitializeNotInitialized);
APS5_EXPORT("sJAiB-6p55U", Hmd2BeforeInitializeNotInitialized);
APS5_EXPORT("E8hDydLjsSs", Hmd2BeforeInitializeNotInitialized);
APS5_EXPORT("xEQkSb2WJ7Y", Hmd2BeforeInitializeNotInitialized);
APS5_EXPORT("aaP4dnR4q-0", Hmd2BeforeInitializeNotInitialized);
APS5_EXPORT("lEU0TeUoGwI", Hmd2BeforeInitializeNotInitialized);
APS5_EXPORT("cFVyjHBpUUU", Hmd2BeforeInitializeNotInitialized);
APS5_EXPORT("zROtb46jTj4", Hmd2BeforeInitializeNotInitialized);

std::int32_t APS5_VABI Hmd2BeforeInitializeError0004() {
    return BeforeInitialize(Hmd2::Error0004, __func__);
}
APS5_EXPORT("kAMsjYHBbNQ", Hmd2BeforeInitializeError0004);
APS5_EXPORT("5m73zmLpsQ0", Hmd2BeforeInitializeError0004);
APS5_EXPORT("EPeH4xuQ2XQ", Hmd2BeforeInitializeError0004);
APS5_EXPORT("TJ8lyRsjFY4", Hmd2BeforeInitializeError0004);
APS5_EXPORT("-XQVcLnyX68", Hmd2BeforeInitializeError0004);
APS5_EXPORT("giQwQoHhYYA", Hmd2BeforeInitializeError0004);
APS5_EXPORT("zNc9IkgawMI", Hmd2BeforeInitializeError0004);
APS5_EXPORT("-tnG5UJPvAc", Hmd2BeforeInitializeError0004);
APS5_EXPORT("FPhFmG+jXuQ", Hmd2BeforeInitializeError0004);
APS5_EXPORT("CKlkvNngofQ", Hmd2BeforeInitializeError0004);
APS5_EXPORT("yL1CZaciFWE", Hmd2BeforeInitializeError0004);
APS5_EXPORT("qlzgyG3s0RY", Hmd2BeforeInitializeError0004);
APS5_EXPORT("kTIDrDLzMv0", Hmd2BeforeInitializeError0004);
APS5_EXPORT("9Gee0ef8a+U", Hmd2BeforeInitializeError0004);
APS5_EXPORT("ecZVjQdwmBI", Hmd2BeforeInitializeError0004);

std::int32_t APS5_VABI Hmd2BeforeInitializeParameterNull() {
    return BeforeInitialize(Hmd2::ParameterNull, __func__);
}
APS5_EXPORT("MNtiJTDOPAU", Hmd2BeforeInitializeParameterNull);
APS5_EXPORT("RRy9WmqZfgU", Hmd2BeforeInitializeParameterNull);
APS5_EXPORT("uobR4m9lgwc", Hmd2BeforeInitializeParameterNull);
APS5_EXPORT("-XPk5KWBYdo", Hmd2BeforeInitializeParameterNull);
APS5_EXPORT("J44idh9izZM", Hmd2BeforeInitializeParameterNull);
APS5_EXPORT("Ps0wAvYaa1E", Hmd2BeforeInitializeParameterNull);
APS5_EXPORT("kLnMR8oQ1MY", Hmd2BeforeInitializeParameterNull);
APS5_EXPORT("kMz4rbBxvjo", Hmd2BeforeInitializeParameterNull);
APS5_EXPORT("DIU1sFRRaiE", Hmd2BeforeInitializeParameterNull);
APS5_EXPORT("ZQvU4vDzZGQ", Hmd2BeforeInitializeParameterNull);
APS5_EXPORT("TJpyGGP9YEY", Hmd2BeforeInitializeParameterNull);
APS5_EXPORT("Dih+Odk+RC0", Hmd2BeforeInitializeParameterNull);
APS5_EXPORT("oaqdSJw+HVY", Hmd2BeforeInitializeParameterNull);
APS5_EXPORT("m35gtHuanXU", Hmd2BeforeInitializeParameterNull);
APS5_EXPORT("CTM7QPLJF9Q", Hmd2BeforeInitializeParameterNull);
APS5_EXPORT("+rExHeC9lP0", Hmd2BeforeInitializeParameterNull);

std::int32_t APS5_VABI Hmd2BeforeInitializeParameterInvalid() {
    return BeforeInitialize(Hmd2::ParameterInvalid, __func__);
}
APS5_EXPORT("bBh5bCp9coo", Hmd2BeforeInitializeParameterInvalid);
APS5_EXPORT("G3PNDE2VjfY", Hmd2BeforeInitializeParameterInvalid);
APS5_EXPORT("nScCT0OPU0o", Hmd2BeforeInitializeParameterInvalid);
APS5_EXPORT("6Q0Ng7OTN9o", Hmd2BeforeInitializeParameterInvalid);
APS5_EXPORT("-fJKh4SnFR0", Hmd2BeforeInitializeParameterInvalid);
APS5_EXPORT("jUA2s-l0npY", Hmd2BeforeInitializeParameterInvalid);
APS5_EXPORT("+9XhyAIJVj4", Hmd2BeforeInitializeParameterInvalid);
APS5_EXPORT("eBGK-o6E4RQ", Hmd2BeforeInitializeParameterInvalid);
APS5_EXPORT("MTuFeL9184E", Hmd2BeforeInitializeParameterInvalid);

std::int32_t APS5_VABI Hmd2BeforeInitializeReprojectionNotInitialized() {
    return BeforeInitialize(Hmd2::ReprojectionNotInitialized, __func__);
}
APS5_EXPORT("d3SEIYTP6wc", Hmd2BeforeInitializeReprojectionNotInitialized);
APS5_EXPORT("EVECbTvBezo", Hmd2BeforeInitializeReprojectionNotInitialized);
APS5_EXPORT("ciusPdJKjSU", Hmd2BeforeInitializeReprojectionNotInitialized);
APS5_EXPORT("DIwqTkcZiJo", Hmd2BeforeInitializeReprojectionNotInitialized);
APS5_EXPORT("64ERZWVFBTE", Hmd2BeforeInitializeReprojectionNotInitialized);
APS5_EXPORT("z0+0gXUJWs8", Hmd2BeforeInitializeReprojectionNotInitialized);
APS5_EXPORT("rjAd4nCY4DM", Hmd2BeforeInitializeReprojectionNotInitialized);
APS5_EXPORT("Acyg4vTdGm0", Hmd2BeforeInitializeReprojectionNotInitialized);
APS5_EXPORT("aGOWaEeg04I", Hmd2BeforeInitializeReprojectionNotInitialized);
APS5_EXPORT("9up+EPhw1No", Hmd2BeforeInitializeReprojectionNotInitialized);

std::int32_t APS5_VABI Hmd2BeforeInitializeError002C() {
    return BeforeInitialize(Hmd2::Error002C, __func__);
}
APS5_EXPORT("V1kLQmP-17U", Hmd2BeforeInitializeError002C);

std::int32_t APS5_VABI Hmd2BeforeInitializeError1002() {
    return BeforeInitialize(Hmd2::Error1002, __func__);
}
APS5_EXPORT("2LVD1EC5SYE", Hmd2BeforeInitializeError1002);
APS5_EXPORT("Bf87AL6zWxU", Hmd2BeforeInitializeError1002);
APS5_EXPORT("XKt7w1MODFY", Hmd2BeforeInitializeError1002);
APS5_EXPORT("Y5s22M-qUDA", Hmd2BeforeInitializeError1002);
APS5_EXPORT("JJKY-BtQJLg", Hmd2BeforeInitializeError1002);

std::int32_t APS5_VABI Hmd2BeforeInitializeError1003() {
    return BeforeInitialize(Hmd2::Error1003, __func__);
}
APS5_EXPORT("9kj9vw8UShQ", Hmd2BeforeInitializeError1003);

std::int32_t APS5_VABI Hmd2BeforeInitializeMinusOne() {
    return BeforeInitialize(-1, __func__);
}
APS5_EXPORT("EAkv8Mqt33E", Hmd2BeforeInitializeMinusOne);
APS5_EXPORT("Rv7bkmKvmz0", Hmd2BeforeInitializeMinusOne);
APS5_EXPORT("1oPZizn0SNA", Hmd2BeforeInitializeMinusOne);
APS5_EXPORT("TwCpE-nNrlo", Hmd2BeforeInitializeMinusOne);
APS5_EXPORT("jkqAeQfdtxw", Hmd2BeforeInitializeMinusOne);
APS5_EXPORT("+l0RQph81Z8", Hmd2BeforeInitializeMinusOne);
APS5_EXPORT("qsmAtGw1Ncc", Hmd2BeforeInitializeMinusOne);
APS5_EXPORT("uKhwYoMsIsA", Hmd2BeforeInitializeMinusOne);
APS5_EXPORT("WNdaigr94dI", Hmd2BeforeInitializeMinusOne);
APS5_EXPORT("P6S5ZVWf-H0", Hmd2BeforeInitializeMinusOne);
APS5_EXPORT("sun6NWE1sdg", Hmd2BeforeInitializeMinusOne);
APS5_EXPORT("--TFsJYXo04", Hmd2BeforeInitializeMinusOne);

std::int32_t APS5_VABI Hmd2BeforeInitializeValue0400() {
    return BeforeInitialize(0x400, __func__);
}
APS5_EXPORT("BWqcwjCozzI", Hmd2BeforeInitializeValue0400);

std::int32_t APS5_VABI Hmd2BeforeInitializeValue20000() {
    return BeforeInitialize(0x20000, __func__);
}
APS5_EXPORT("PRz7TdBGUI0", Hmd2BeforeInitializeValue20000);

int APS5_VABI sceHmd2InternalSetPortStatus(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2InternalLedDetectorStartBySlot(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2InternalImageStartBySlot(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2InternalSetPortConnection(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2InternalSetCameraSeeThroughMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2InternalSetFakeLensSeparationDistance(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2InternalDfuSend(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2InternalLedDetectorOpenBySlot(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2InternalImageOpenBySlot(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2InternalLedDetectorStopBySlot(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2InternalSetDeviceConnection(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2InternalLedDetectorCloseBySlot(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2InternalSetSidetone(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2InternalGetDeviceId(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2InternalSetBrightness(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2InternalImageStopBySlot(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2InternalSetVibrationMode(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2InternalGetVr2dData(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

int APS5_VABI sceHmd2InternalImageCloseBySlot(void) {
    NotImplemented_nid_no_patch(__func__);
    return 0;
}

}
