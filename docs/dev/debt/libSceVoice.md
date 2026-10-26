# libSceVoice technical debt

### Unknown function info

- [sceVoiceResetPort](../../../core/libs/prx/libSceVoice/Export.cpp) (libSceVoice) - the `(uint32_t portId)` signature is the PS Vita one in [VitaSDK](https://github.com/vitasdk/vita-headers/blob/ae5808f3105c42fa3373ba4ab64ab54e2e3ca848/include/psp2/voice.h#L503-L514) and the PS3 `cellVoiceResetPort` in [RPCS3](https://github.com/RPCS3/rpcs3/blob/be69c054eeac57e2e671697a463473d37fe5a7b2/rpcs3/Emu/Cell/Modules/cellVoice.cpp#L493-L510); that the PS5 library takes the same argument is assumed. A reset clears a port's buffered data, and ports here buffer nothing (`sceVoiceReadFromOPort` returns 0 bytes and `sceVoiceWriteToIPort` consumes nothing (writes `*size = 0`)), so it only checks that the library is initialized and the port exists, as RPCS3 does, and keeps the port's connections, volume and mute flag. An uninitialized library or an unknown port throws like the other port calls; the console's error codes are not measured. PPSA23566 imports it

### Silent stubs

- [sceVoiceWriteToIPort](../../../core/libs/prx/libSceVoice/Export.cpp) (libSceVoice) - validates the port and arguments then discards every byte: nothing is buffered between the caller and `sceVoiceReadFromOPort`, which still reports 0 bytes. It writes `*size = 0` on success so a title does not mistake discarded audio for consumed audio, matching the 0-byte convention already used by `sceVoiceReadFromOPort`. No reference (KytyPS5, shadPS4, prosper) carries a body for this call, so the exact in/out `*size` contract on a real console is unknown; if measurement later shows a different convention, update this line alongside the implementation.
