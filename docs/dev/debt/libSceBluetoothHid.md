# libSceBluetoothHid technical debt

### Silent stubs

- [libSceBluetoothHid](../../../core/libs/prx/libSceBluetoothHid/Export.cpp) implements only the no-device path: initialization and callback and device registration succeed, the callback is never called because no device connects, and calls that need a connected device throw

### Unknown function info

- [sceBluetoothHidDebugGetVersion, sceBluetoothHidDisconnectDevice, sceBluetoothHidGetDeviceInfo, sceBluetoothHidGetFeatureReport, sceBluetoothHidGetReportDescriptor, sceBluetoothHidInterruptOutput, sceBluetoothHidParamInitialize, sceBluetoothHidSetFeatureReport and sceBluetoothHidThreadParamInitialize](../../../core/libs/prx/libSceBluetoothHid/Export.cpp) (libSceBluetoothHid) - names and NIDs are present in the [PS5 3.20 export inventory](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libSceBluetoothHid.c); parameter lists follow [SharpProspero's Bluetooth HID bindings](https://github.com/SvenGDK/SharpProspero/blob/5dfdb4b0b166e83dc9ae275001afb695f12c7174/src/SharpProspero/Interop/Bluetooth/SceBluetoothHid.cs#L39-L112), not measured on a console. These exports are throwing declarations to resolve known imports.

- [libSceBluetoothHid](../../../core/libs/prx/libSceBluetoothHid/Export.cpp) (libSceBluetoothHid) - signatures taken from PPSA01317: `sceBluetoothHidInit()` and `sceBluetoothHidUnregisterCallback()` without arguments, `sceBluetoothHidRegisterCallback(callback, 0, param)` with a 16-byte param, and `sceBluetoothHidRegisterDevice` / `sceBluetoothHidUnregisterDevice(vendorId, productId)`; the report and device name exports are declared without parameters
- [sceBluetoothHidDebugGetVersion, sceBluetoothHidDisconnectDevice, sceBluetoothHidGetDeviceInfo, sceBluetoothHidGetFeatureReport, sceBluetoothHidGetReportDescriptor, sceBluetoothHidInterruptOutput, sceBluetoothHidParamInitialize, sceBluetoothHidSetFeatureReport, sceBluetoothHidThreadParamInitialize](../../../core/libs/prx/libSceBluetoothHid/Export.cpp) (libSceBluetoothHid) - throwing stubs declared without parameters from the firmware export database (aerolib.csv); their guest signatures are unverified on hardware
