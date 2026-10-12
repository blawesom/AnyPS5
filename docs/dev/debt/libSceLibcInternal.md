# libSceLibcInternal technical debt

### Unknown function info

- [_sceLibcInternalThreadAtexit, _sceLibcInternalThreadDtors](../../../core/libs/prx/libSceLibcInternal/Export.cpp) (libSceLibcInternal) - FreeBSD `__cxa_thread_atexit` semantics assumed; a null or non-image destructor and an allocation failure throw
- [_sceLibcInternalForceTlsDestructor](../../../core/libs/prx/libSceLibcInternal/Export.cpp) (libSceLibcInternal) - behaviour follows `sceLibcForceTlsDestructor(int)` in a title's `libc.prx` (disassembly): the calling thread's destructors whose dso belongs to the module run newest first, only if the dso's module is loaded and the dso points to itself, in up to 4 passes. The return type is unknown (that function returns nothing), 0 is returned. The module of each dso is looked up when the destructors are forced, not when they are registered as `libc.prx` does

### Functional

- [makecontext](../../../core/libs/prx/libSceLibcInternal/UserContext.cpp) (libSceLibcInternal) follows FreeBSD 11's `lib/libc/amd64/gen/makecontext.c`: at most six arguments, a stack of at least `MINSIGSTKSZ` (2048) bytes, and `exit(0)` when the function returns with no `uc_link`. The console's version is not verified
- [makecontext](../../../core/libs/prx/libSceLibcInternal/UserContext.cpp) (libSceLibcInternal) on Windows pins its stack writable until a later `makecontext` stack overlaps it, since nothing tells the library when the guest frees the stack
- [Thread exit destructors](../../../core/libs/prx/libSceLibcInternal/Export.cpp) (libSceLibcInternal) run when a libkernel thread finishes; on the main or a host thread only `_sceLibcInternalThreadDtors` runs them, as `exit` does, else they are dropped
- [fstatvfs](../../../core/libs/prx/libSceLibcInternal/FilesystemStatistics.cpp) (libSceLibcInternal) converts `_fstatfs` to the 88-byte `struct statvfs` as FreeBSD 9.2's `sfs2svfs` does (`f_bsize` from the transfer size, `f_frsize` from the block size, `f_favail` equal to `f_ffree`, `f_fsid` 0, `ST_RDONLY` and `ST_NOSUID` only). Differences: `f_namemax` is the `statfs` value, not `fpathconf(_PC_NAME_MAX)`; negative `f_bavail` and `f_ffree` wrap to unsigned without `EOVERFLOW`, as the same-width copy in FreeBSD does; inode counts are 0 on Windows, inherited from `_fstatfs`; a null output returns `EFAULT` after the descriptor is validated, where FreeBSD would fault. Layout and conversion are unverified against the PS5 library (PS5MC imports `fstatvfs` but was not run)
