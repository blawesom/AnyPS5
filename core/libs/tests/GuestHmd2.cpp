#include "ProcessParametersFixture.hpp"
#include "prx/libSceHmd/Hmd.hpp"
#include "prx/libSceHmd2/Hmd2.hpp"
#include "prx/libSceUserService/UserService.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

extern "C" {
std::int32_t APS5_VABI Hmd2CheckFirstThenError0004(const void*);
std::int32_t APS5_VABI Hmd2CheckFirstThenZero(const void*);
std::int32_t APS5_VABI Hmd2CheckFirstThenError1002(const void*);
std::int32_t APS5_VABI Hmd2CheckFirstThenMinusOne(const void*);
std::int32_t APS5_VABI Hmd2CheckFirstTwoThenParameterInvalid(const void*, const void*);
std::int32_t APS5_VABI Hmd2CheckSecondThenError002C(const void*, const void*);
std::int32_t APS5_VABI Hmd2CheckFirstThenParameterInvalid(const void*);
std::int32_t APS5_VABI Hmd2CheckAllSixThenZero(const void*, const void*, const void*, const void*, const void*, const void*);
std::int32_t APS5_VABI Hmd2CheckSecondThenParameterInvalid(const void*, const void*);
std::int32_t APS5_VABI Hmd2CheckFirstThenError800d0203(const void*);
std::int32_t APS5_VABI Hmd2CheckFirstThenError1003(const void*);
std::int32_t APS5_VABI Hmd2CheckFirstThenError1010(const void*);
std::int32_t APS5_VABI Hmd2CheckFirstTwoThenError1002(const void*, const void*);
std::int32_t APS5_VABI Hmd2CheckFirstTwoThenError1003(const void*, const void*);
std::int32_t APS5_VABI Hmd2CheckFirstAndThirdThenZero(const void*, const void*, const void*);
std::int32_t APS5_VABI Hmd2CheckFirstAndThirdThenError1003(const void*, const void*, const void*);
std::int32_t APS5_VABI Hmd2CheckSecondThenInvalidHandle(const void*, const void*);
std::int32_t APS5_VABI Hmd2CheckSecondThenError1002(const void*, const void*);
std::int32_t APS5_VABI Hmd2BeforeInitializeZero();
std::int32_t APS5_VABI Hmd2BeforeInitializeNotInitialized();
std::int32_t APS5_VABI Hmd2BeforeInitializeError0004();
std::int32_t APS5_VABI Hmd2BeforeInitializeParameterNull();
std::int32_t APS5_VABI Hmd2BeforeInitializeParameterInvalid();
std::int32_t APS5_VABI Hmd2BeforeInitializeReprojectionNotInitialized();
std::int32_t APS5_VABI Hmd2BeforeInitializeError002C();
std::int32_t APS5_VABI Hmd2BeforeInitializeError1002();
std::int32_t APS5_VABI Hmd2BeforeInitializeError1003();
std::int32_t APS5_VABI Hmd2BeforeInitializeMinusOne();
std::int32_t APS5_VABI Hmd2BeforeInitializeValue0400();
std::int32_t APS5_VABI Hmd2BeforeInitializeValue20000();
}

namespace {

alignas(16) std::array<std::byte, 0x40> process{};

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "HMD2: %s\n", message);
        std::abort();
    }
}

void InstallProcessParameters() {
    const std::uint64_t size = process.size();
    const std::uint32_t magic = 0x4942524f;
    std::memcpy(process.data(), &size, sizeof(size));
    std::memcpy(process.data() + 8, &magic, sizeof(magic));
    Require(ProcessParametersFixture::Install(process.data(), process.size()), "process parameters were not installed");
}

void TestBeforeInitialize() {
    Hmd2::DeviceInformation info;
    std::memset(&info, 0xcc, sizeof(info));
    Require(sceHmd2GetDeviceInformation(&info) == Hmd2::NotInitialized, "device information before initialization");
    const Hmd2::DeviceInformation cleared{};
    Require(std::memcmp(&info, &cleared, sizeof(info)) == 0, "device information was not cleared before the initialization check");
    Require(sceHmd2GetDeviceInformation(nullptr) == Hmd2::ParameterNull, "null device information before initialization");
    Hmd2::FieldOfView fieldOfView{};
    Require(sceHmd2GetFieldOfViewWithoutHandle(&fieldOfView) == Hmd2::NotInitialized, "field of view before initialization");
    Require(sceHmd2GetFieldOfView(0, &fieldOfView) == Hmd2::InvalidHandle, "field of view of handle 0 before initialization");
    Require(sceHmd2GetDeviceInformationByHandle(0, &info) == Hmd2::NotInitialized, "device information of handle 0 before initialization");
    Require(sceHmd2Open(USER_SERVICE_INITIAL_USER_ID, 0, 0, nullptr) == Hmd2::NotInitialized, "open before initialization");
    Require(sceHmd2Close(0) == Hmd2::NotInitialized, "close before initialization");
    Require(sceHmd2Terminate() == Hmd2::NotInitialized, "terminate before initialization");
    const std::uint8_t dummyBytes[16]{};
    const void* dummy = dummyBytes;
    Require(Hmd2CheckFirstThenError0004(dummy) == Hmd2::Error0004, "Hmd2CheckFirstThenError0004 with every pointer present before initialization");
    Require(Hmd2CheckFirstThenError0004(nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstThenError0004 with pointer 0 null before initialization");
    Require(Hmd2CheckFirstThenZero(dummy) == 0, "Hmd2CheckFirstThenZero with every pointer present before initialization");
    Require(Hmd2CheckFirstThenZero(nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstThenZero with pointer 0 null before initialization");
    Require(Hmd2CheckFirstThenError1002(dummy) == Hmd2::Error1002, "Hmd2CheckFirstThenError1002 with every pointer present before initialization");
    Require(Hmd2CheckFirstThenError1002(nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstThenError1002 with pointer 0 null before initialization");
    Require(Hmd2CheckFirstThenMinusOne(dummy) == -1, "Hmd2CheckFirstThenMinusOne with every pointer present before initialization");
    Require(Hmd2CheckFirstThenMinusOne(nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstThenMinusOne with pointer 0 null before initialization");
    Require(Hmd2CheckFirstTwoThenParameterInvalid(dummy, dummy) == Hmd2::ParameterInvalid, "Hmd2CheckFirstTwoThenParameterInvalid with every pointer present before initialization");
    Require(Hmd2CheckFirstTwoThenParameterInvalid(nullptr, dummy) == Hmd2::ParameterNull, "Hmd2CheckFirstTwoThenParameterInvalid with pointer 0 null before initialization");
    Require(Hmd2CheckFirstTwoThenParameterInvalid(dummy, nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstTwoThenParameterInvalid with pointer 1 null before initialization");
    Require(Hmd2CheckSecondThenError002C(dummy, dummy) == Hmd2::Error002C, "Hmd2CheckSecondThenError002C with every pointer present before initialization");
    Require(Hmd2CheckSecondThenError002C(dummy, nullptr) == Hmd2::ParameterNull, "Hmd2CheckSecondThenError002C with pointer 1 null before initialization");
    Require(Hmd2CheckFirstThenParameterInvalid(dummy) == Hmd2::ParameterInvalid, "Hmd2CheckFirstThenParameterInvalid with every pointer present before initialization");
    Require(Hmd2CheckFirstThenParameterInvalid(nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstThenParameterInvalid with pointer 0 null before initialization");
    Require(Hmd2CheckAllSixThenZero(dummy, dummy, dummy, dummy, dummy, dummy) == 0, "Hmd2CheckAllSixThenZero with every pointer present before initialization");
    Require(Hmd2CheckAllSixThenZero(nullptr, dummy, dummy, dummy, dummy, dummy) == Hmd2::ParameterNull, "Hmd2CheckAllSixThenZero with pointer 0 null before initialization");
    Require(Hmd2CheckAllSixThenZero(dummy, nullptr, dummy, dummy, dummy, dummy) == Hmd2::ParameterNull, "Hmd2CheckAllSixThenZero with pointer 1 null before initialization");
    Require(Hmd2CheckAllSixThenZero(dummy, dummy, nullptr, dummy, dummy, dummy) == Hmd2::ParameterNull, "Hmd2CheckAllSixThenZero with pointer 2 null before initialization");
    Require(Hmd2CheckAllSixThenZero(dummy, dummy, dummy, nullptr, dummy, dummy) == Hmd2::ParameterNull, "Hmd2CheckAllSixThenZero with pointer 3 null before initialization");
    Require(Hmd2CheckAllSixThenZero(dummy, dummy, dummy, dummy, nullptr, dummy) == Hmd2::ParameterNull, "Hmd2CheckAllSixThenZero with pointer 4 null before initialization");
    Require(Hmd2CheckAllSixThenZero(dummy, dummy, dummy, dummy, dummy, nullptr) == Hmd2::ParameterNull, "Hmd2CheckAllSixThenZero with pointer 5 null before initialization");
    Require(Hmd2CheckSecondThenParameterInvalid(dummy, dummy) == Hmd2::ParameterInvalid, "Hmd2CheckSecondThenParameterInvalid with every pointer present before initialization");
    Require(Hmd2CheckSecondThenParameterInvalid(dummy, nullptr) == Hmd2::ParameterNull, "Hmd2CheckSecondThenParameterInvalid with pointer 1 null before initialization");
    Require(Hmd2CheckFirstThenError800d0203(dummy) == Hmd2::Error800d0203, "Hmd2CheckFirstThenError800d0203 with every pointer present before initialization");
    Require(Hmd2CheckFirstThenError800d0203(nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstThenError800d0203 with pointer 0 null before initialization");
    Require(Hmd2CheckFirstThenError1003(dummy) == Hmd2::Error1003, "Hmd2CheckFirstThenError1003 with every pointer present before initialization");
    Require(Hmd2CheckFirstThenError1003(nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstThenError1003 with pointer 0 null before initialization");
    Require(Hmd2CheckFirstThenError1010(dummy) == Hmd2::Error1010, "Hmd2CheckFirstThenError1010 with every pointer present before initialization");
    Require(Hmd2CheckFirstThenError1010(nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstThenError1010 with pointer 0 null before initialization");
    Require(Hmd2CheckFirstTwoThenError1002(dummy, dummy) == Hmd2::Error1002, "Hmd2CheckFirstTwoThenError1002 with every pointer present before initialization");
    Require(Hmd2CheckFirstTwoThenError1002(nullptr, dummy) == Hmd2::ParameterNull, "Hmd2CheckFirstTwoThenError1002 with pointer 0 null before initialization");
    Require(Hmd2CheckFirstTwoThenError1002(dummy, nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstTwoThenError1002 with pointer 1 null before initialization");
    Require(Hmd2CheckFirstTwoThenError1003(dummy, dummy) == Hmd2::Error1003, "Hmd2CheckFirstTwoThenError1003 with every pointer present before initialization");
    Require(Hmd2CheckFirstTwoThenError1003(nullptr, dummy) == Hmd2::ParameterNull, "Hmd2CheckFirstTwoThenError1003 with pointer 0 null before initialization");
    Require(Hmd2CheckFirstTwoThenError1003(dummy, nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstTwoThenError1003 with pointer 1 null before initialization");
    Require(Hmd2CheckFirstAndThirdThenZero(dummy, dummy, dummy) == 0, "Hmd2CheckFirstAndThirdThenZero with every pointer present before initialization");
    Require(Hmd2CheckFirstAndThirdThenZero(nullptr, dummy, dummy) == Hmd2::ParameterNull, "Hmd2CheckFirstAndThirdThenZero with pointer 0 null before initialization");
    Require(Hmd2CheckFirstAndThirdThenZero(dummy, dummy, nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstAndThirdThenZero with pointer 2 null before initialization");
    Require(Hmd2CheckFirstAndThirdThenError1003(dummy, dummy, dummy) == Hmd2::Error1003, "Hmd2CheckFirstAndThirdThenError1003 with every pointer present before initialization");
    Require(Hmd2CheckFirstAndThirdThenError1003(nullptr, dummy, dummy) == Hmd2::ParameterNull, "Hmd2CheckFirstAndThirdThenError1003 with pointer 0 null before initialization");
    Require(Hmd2CheckFirstAndThirdThenError1003(dummy, dummy, nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstAndThirdThenError1003 with pointer 2 null before initialization");
    Require(Hmd2CheckSecondThenInvalidHandle(dummy, dummy) == Hmd2::InvalidHandle, "Hmd2CheckSecondThenInvalidHandle with every pointer present before initialization");
    Require(Hmd2CheckSecondThenInvalidHandle(dummy, nullptr) == Hmd2::ParameterNull, "Hmd2CheckSecondThenInvalidHandle with pointer 1 null before initialization");
    Require(Hmd2CheckSecondThenError1002(dummy, dummy) == Hmd2::Error1002, "Hmd2CheckSecondThenError1002 with every pointer present before initialization");
    Require(Hmd2CheckSecondThenError1002(dummy, nullptr) == Hmd2::ParameterNull, "Hmd2CheckSecondThenError1002 with pointer 1 null before initialization");
    Require(Hmd2BeforeInitializeZero() == 0, "Hmd2BeforeInitializeZero before initialization");
    Require(Hmd2BeforeInitializeNotInitialized() == Hmd2::NotInitialized, "Hmd2BeforeInitializeNotInitialized before initialization");
    Require(Hmd2BeforeInitializeError0004() == Hmd2::Error0004, "Hmd2BeforeInitializeError0004 before initialization");
    Require(Hmd2BeforeInitializeParameterNull() == Hmd2::ParameterNull, "Hmd2BeforeInitializeParameterNull before initialization");
    Require(Hmd2BeforeInitializeParameterInvalid() == Hmd2::ParameterInvalid, "Hmd2BeforeInitializeParameterInvalid before initialization");
    Require(Hmd2BeforeInitializeReprojectionNotInitialized() == Hmd2::ReprojectionNotInitialized, "Hmd2BeforeInitializeReprojectionNotInitialized before initialization");
    Require(Hmd2BeforeInitializeError002C() == Hmd2::Error002C, "Hmd2BeforeInitializeError002C before initialization");
    Require(Hmd2BeforeInitializeError1002() == Hmd2::Error1002, "Hmd2BeforeInitializeError1002 before initialization");
    Require(Hmd2BeforeInitializeError1003() == Hmd2::Error1003, "Hmd2BeforeInitializeError1003 before initialization");
    Require(Hmd2BeforeInitializeMinusOne() == -1, "Hmd2BeforeInitializeMinusOne before initialization");
    Require(Hmd2BeforeInitializeValue0400() == 0x400, "Hmd2BeforeInitializeValue0400 before initialization");
    Require(Hmd2BeforeInitializeValue20000() == 0x20000, "Hmd2BeforeInitializeValue20000 before initialization");
    Require(sceHmd2SetVibration(0, 0) == 0, "vibration of the unopened handle 0 failed");
}

void TestWithoutHeadset() {
    const std::uint8_t param[16]{};
    Require(sceHmd2Initialize(param) == Hmd::UnsupportedFeature, "initialization without a headset did not fail");
    Require(sceHmd2Initialize(nullptr) == Hmd::UnsupportedFeature, "initialization without a param did not fail as unsupported");
    Require(sceHmd2Open(USER_SERVICE_INITIAL_USER_ID, 0, 0, nullptr) == Hmd2::NotInitialized, "open without a headset did not fail");
    Hmd2::DeviceInformation info;
    Require(sceHmd2GetDeviceInformation(&info) == Hmd2::NotInitialized, "device information without a headset");
    Require(sceHmd2Terminate() == Hmd2::NotInitialized, "terminate after the failed initialization");
    const std::uint8_t dummyBytes[16]{};
    const void* dummy = dummyBytes;
    Require(Hmd2CheckFirstThenError0004(dummy) == Hmd2::Error0004, "Hmd2CheckFirstThenError0004 with every pointer present after the failed initialization");
    Require(Hmd2CheckFirstThenError0004(nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstThenError0004 with pointer 0 null after the failed initialization");
    Require(Hmd2CheckFirstThenZero(dummy) == 0, "Hmd2CheckFirstThenZero with every pointer present after the failed initialization");
    Require(Hmd2CheckFirstThenZero(nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstThenZero with pointer 0 null after the failed initialization");
    Require(Hmd2CheckFirstThenError1002(dummy) == Hmd2::Error1002, "Hmd2CheckFirstThenError1002 with every pointer present after the failed initialization");
    Require(Hmd2CheckFirstThenError1002(nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstThenError1002 with pointer 0 null after the failed initialization");
    Require(Hmd2CheckFirstThenMinusOne(dummy) == -1, "Hmd2CheckFirstThenMinusOne with every pointer present after the failed initialization");
    Require(Hmd2CheckFirstThenMinusOne(nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstThenMinusOne with pointer 0 null after the failed initialization");
    Require(Hmd2CheckFirstTwoThenParameterInvalid(dummy, dummy) == Hmd2::ParameterInvalid, "Hmd2CheckFirstTwoThenParameterInvalid with every pointer present after the failed initialization");
    Require(Hmd2CheckFirstTwoThenParameterInvalid(nullptr, dummy) == Hmd2::ParameterNull, "Hmd2CheckFirstTwoThenParameterInvalid with pointer 0 null after the failed initialization");
    Require(Hmd2CheckFirstTwoThenParameterInvalid(dummy, nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstTwoThenParameterInvalid with pointer 1 null after the failed initialization");
    Require(Hmd2CheckSecondThenError002C(dummy, dummy) == Hmd2::Error002C, "Hmd2CheckSecondThenError002C with every pointer present after the failed initialization");
    Require(Hmd2CheckSecondThenError002C(dummy, nullptr) == Hmd2::ParameterNull, "Hmd2CheckSecondThenError002C with pointer 1 null after the failed initialization");
    Require(Hmd2CheckFirstThenParameterInvalid(dummy) == Hmd2::ParameterInvalid, "Hmd2CheckFirstThenParameterInvalid with every pointer present after the failed initialization");
    Require(Hmd2CheckFirstThenParameterInvalid(nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstThenParameterInvalid with pointer 0 null after the failed initialization");
    Require(Hmd2CheckAllSixThenZero(dummy, dummy, dummy, dummy, dummy, dummy) == 0, "Hmd2CheckAllSixThenZero with every pointer present after the failed initialization");
    Require(Hmd2CheckAllSixThenZero(nullptr, dummy, dummy, dummy, dummy, dummy) == Hmd2::ParameterNull, "Hmd2CheckAllSixThenZero with pointer 0 null after the failed initialization");
    Require(Hmd2CheckAllSixThenZero(dummy, nullptr, dummy, dummy, dummy, dummy) == Hmd2::ParameterNull, "Hmd2CheckAllSixThenZero with pointer 1 null after the failed initialization");
    Require(Hmd2CheckAllSixThenZero(dummy, dummy, nullptr, dummy, dummy, dummy) == Hmd2::ParameterNull, "Hmd2CheckAllSixThenZero with pointer 2 null after the failed initialization");
    Require(Hmd2CheckAllSixThenZero(dummy, dummy, dummy, nullptr, dummy, dummy) == Hmd2::ParameterNull, "Hmd2CheckAllSixThenZero with pointer 3 null after the failed initialization");
    Require(Hmd2CheckAllSixThenZero(dummy, dummy, dummy, dummy, nullptr, dummy) == Hmd2::ParameterNull, "Hmd2CheckAllSixThenZero with pointer 4 null after the failed initialization");
    Require(Hmd2CheckAllSixThenZero(dummy, dummy, dummy, dummy, dummy, nullptr) == Hmd2::ParameterNull, "Hmd2CheckAllSixThenZero with pointer 5 null after the failed initialization");
    Require(Hmd2CheckSecondThenParameterInvalid(dummy, dummy) == Hmd2::ParameterInvalid, "Hmd2CheckSecondThenParameterInvalid with every pointer present after the failed initialization");
    Require(Hmd2CheckSecondThenParameterInvalid(dummy, nullptr) == Hmd2::ParameterNull, "Hmd2CheckSecondThenParameterInvalid with pointer 1 null after the failed initialization");
    Require(Hmd2CheckFirstThenError800d0203(dummy) == Hmd2::Error800d0203, "Hmd2CheckFirstThenError800d0203 with every pointer present after the failed initialization");
    Require(Hmd2CheckFirstThenError800d0203(nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstThenError800d0203 with pointer 0 null after the failed initialization");
    Require(Hmd2CheckFirstThenError1003(dummy) == Hmd2::Error1003, "Hmd2CheckFirstThenError1003 with every pointer present after the failed initialization");
    Require(Hmd2CheckFirstThenError1003(nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstThenError1003 with pointer 0 null after the failed initialization");
    Require(Hmd2CheckFirstThenError1010(dummy) == Hmd2::Error1010, "Hmd2CheckFirstThenError1010 with every pointer present after the failed initialization");
    Require(Hmd2CheckFirstThenError1010(nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstThenError1010 with pointer 0 null after the failed initialization");
    Require(Hmd2CheckFirstTwoThenError1002(dummy, dummy) == Hmd2::Error1002, "Hmd2CheckFirstTwoThenError1002 with every pointer present after the failed initialization");
    Require(Hmd2CheckFirstTwoThenError1002(nullptr, dummy) == Hmd2::ParameterNull, "Hmd2CheckFirstTwoThenError1002 with pointer 0 null after the failed initialization");
    Require(Hmd2CheckFirstTwoThenError1002(dummy, nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstTwoThenError1002 with pointer 1 null after the failed initialization");
    Require(Hmd2CheckFirstTwoThenError1003(dummy, dummy) == Hmd2::Error1003, "Hmd2CheckFirstTwoThenError1003 with every pointer present after the failed initialization");
    Require(Hmd2CheckFirstTwoThenError1003(nullptr, dummy) == Hmd2::ParameterNull, "Hmd2CheckFirstTwoThenError1003 with pointer 0 null after the failed initialization");
    Require(Hmd2CheckFirstTwoThenError1003(dummy, nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstTwoThenError1003 with pointer 1 null after the failed initialization");
    Require(Hmd2CheckFirstAndThirdThenZero(dummy, dummy, dummy) == 0, "Hmd2CheckFirstAndThirdThenZero with every pointer present after the failed initialization");
    Require(Hmd2CheckFirstAndThirdThenZero(nullptr, dummy, dummy) == Hmd2::ParameterNull, "Hmd2CheckFirstAndThirdThenZero with pointer 0 null after the failed initialization");
    Require(Hmd2CheckFirstAndThirdThenZero(dummy, dummy, nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstAndThirdThenZero with pointer 2 null after the failed initialization");
    Require(Hmd2CheckFirstAndThirdThenError1003(dummy, dummy, dummy) == Hmd2::Error1003, "Hmd2CheckFirstAndThirdThenError1003 with every pointer present after the failed initialization");
    Require(Hmd2CheckFirstAndThirdThenError1003(nullptr, dummy, dummy) == Hmd2::ParameterNull, "Hmd2CheckFirstAndThirdThenError1003 with pointer 0 null after the failed initialization");
    Require(Hmd2CheckFirstAndThirdThenError1003(dummy, dummy, nullptr) == Hmd2::ParameterNull, "Hmd2CheckFirstAndThirdThenError1003 with pointer 2 null after the failed initialization");
    Require(Hmd2CheckSecondThenInvalidHandle(dummy, dummy) == Hmd2::InvalidHandle, "Hmd2CheckSecondThenInvalidHandle with every pointer present after the failed initialization");
    Require(Hmd2CheckSecondThenInvalidHandle(dummy, nullptr) == Hmd2::ParameterNull, "Hmd2CheckSecondThenInvalidHandle with pointer 1 null after the failed initialization");
    Require(Hmd2CheckSecondThenError1002(dummy, dummy) == Hmd2::Error1002, "Hmd2CheckSecondThenError1002 with every pointer present after the failed initialization");
    Require(Hmd2CheckSecondThenError1002(dummy, nullptr) == Hmd2::ParameterNull, "Hmd2CheckSecondThenError1002 with pointer 1 null after the failed initialization");
    Require(Hmd2BeforeInitializeZero() == 0, "Hmd2BeforeInitializeZero after the failed initialization");
    Require(Hmd2BeforeInitializeNotInitialized() == Hmd2::NotInitialized, "Hmd2BeforeInitializeNotInitialized after the failed initialization");
    Require(Hmd2BeforeInitializeError0004() == Hmd2::Error0004, "Hmd2BeforeInitializeError0004 after the failed initialization");
    Require(Hmd2BeforeInitializeParameterNull() == Hmd2::ParameterNull, "Hmd2BeforeInitializeParameterNull after the failed initialization");
    Require(Hmd2BeforeInitializeParameterInvalid() == Hmd2::ParameterInvalid, "Hmd2BeforeInitializeParameterInvalid after the failed initialization");
    Require(Hmd2BeforeInitializeReprojectionNotInitialized() == Hmd2::ReprojectionNotInitialized, "Hmd2BeforeInitializeReprojectionNotInitialized after the failed initialization");
    Require(Hmd2BeforeInitializeError002C() == Hmd2::Error002C, "Hmd2BeforeInitializeError002C after the failed initialization");
    Require(Hmd2BeforeInitializeError1002() == Hmd2::Error1002, "Hmd2BeforeInitializeError1002 after the failed initialization");
    Require(Hmd2BeforeInitializeError1003() == Hmd2::Error1003, "Hmd2BeforeInitializeError1003 after the failed initialization");
    Require(Hmd2BeforeInitializeMinusOne() == -1, "Hmd2BeforeInitializeMinusOne after the failed initialization");
    Require(Hmd2BeforeInitializeValue0400() == 0x400, "Hmd2BeforeInitializeValue0400 after the failed initialization");
    Require(Hmd2BeforeInitializeValue20000() == 0x20000, "Hmd2BeforeInitializeValue20000 after the failed initialization");
}

}

void SizeQueriesReturnTheConsoleSizes() {
    const auto display = sceHmd2ReprojectionQueryDisplayBufferSizeAlign();
    Require(display.size == 0x6000000 && display.align == 0x10000, "display buffer size");
    const auto seeThrough = sceHmd2ReprojectionQuerySeeThroughBufferSizeAlign();
    Require(seeThrough.size == 0x1106000 && seeThrough.align == 0x100, "see-through buffer size");
    const auto mirroring = sceHmd2ReprojectionGetMirroringDisplayBufferSizeAlign();
    Require(mirroring.size == 0x5FA0000 && mirroring.align == 0x10000, "mirroring display buffer size");
    const auto work = sceHmd2ReprojectionGetMirroringWorkMemorySizeAlign();
    Require(work.size == 0x4000 && work.align == 0x100, "mirroring work memory size");
}

int main() {
    SizeQueriesReturnTheConsoleSizes();
    InstallProcessParameters();
    TestBeforeInitialize();
    TestWithoutHeadset();
    std::puts("HMD2 tests passed");
    return 0;
}
