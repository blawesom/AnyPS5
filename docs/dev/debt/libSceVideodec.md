# libSceVideodec technical debt

### Unknown function info

- [sceVideodecMapMemory](../../../core/libs/prx/libSceVideodec/Export.cpp) (libSceVideodec) - its name and NID are present in the [PS5 3.20 export inventory](https://github.com/DNNDHH/PS5-3.20_Libs/blob/b2a77c09cd71c01f34c790cfbe98c751a13262ad/libSceVideodec.c); the no-parameter prototype follows [shadPS4's videodec.cpp](https://github.com/shadps4-emu/shadPS4/blob/f68923fe807f657348e6719ce658bcbd0a2b1c72/src/core/libraries/videodec/videodec.cpp#L132), not measured on a console.
- [sceVideodecQueryResourceInfo](../../../core/libs/prx/libSceVideodec/Export.cpp) (libSceVideodec) - its `(const SceVideodecConfigInfo*, SceVideodecResourceInfo*)` prototype follows [ChonkyStation4's SceVideodec.cpp](https://github.com/liuk7071/ChonkyStation4/blob/c101a23ecbc15e56c6e30d5dd24d3ed8880a957a/ChonkyStation4/OS/Libraries/SceVideodec/SceVideodec.cpp#L31), not measured on a console.
