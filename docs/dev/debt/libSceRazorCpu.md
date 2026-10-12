# libSceRazorCpu technical debt

### Silent stubs

- [sceRazorCpuPushMarker](../../../core/libs/prx/libSceRazorCpu/Export.cpp) (libSceRazorCpu) - signature (label, color, flags) from [SharpProspero](https://github.com/SvenGDK/SharpProspero), the same as sceRazorCpuPushMarkerStatic's except that the label is copied; it does nothing without a capture and returns 0

### Unknown function info

- [sceRazorCpuFlushOccurred](../../../core/libs/prx/libSceRazorCpu/Export.cpp) (libSceRazorCpu) - signature and return values (1 = flush since the last call, 0 = none) from [SharpProspero](https://github.com/SvenGDK/SharpProspero); without a capture nothing is flushed, so it returns 0 and writes 0 cycles to the optional out pointer (whether the real library writes it when nothing was flushed is unknown)
- [sceRazorCpuPushMarkerStatic, sceRazorCpuPopMarker](../../../core/libs/prx/libSceRazorCpu/Export.cpp) (libSceRazorCpu) - signatures from PPSA26344's calls; (name, color, flags) assumed for the push; both do nothing without a capture
- [sceRazorCpuPlotValue, sceRazorCpuWriteBookmark](../../../core/libs/prx/libSceRazorCpu/Export.cpp) (libSceRazorCpu) - signatures (series, float value) and (label, optional description) from [SharpProspero](https://github.com/SvenGDK/SharpProspero), matching PPSA06320's calls; both do nothing without a capture and return 0
