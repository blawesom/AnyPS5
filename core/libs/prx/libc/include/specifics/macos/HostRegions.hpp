#ifndef CORE_LIBS_PRX_LIBC_INCLUDE_SPECIFICS_MACOS_HOSTREGIONS_HPP
#define CORE_LIBS_PRX_LIBC_INCLUDE_SPECIFICS_MACOS_HOSTREGIONS_HPP

#ifdef __APPLE__

#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <cstdint>

namespace HostRegions {

struct Region {
    std::uintptr_t begin = 0;
    std::uintptr_t end = 0;
    bool readable = false;
    bool writable = false;
    bool executable = false;
};

inline bool FirstAtOrAbove(const std::uintptr_t address, Region& region) {
    mach_vm_address_t start = address;
    mach_vm_size_t size = 0;
    vm_region_basic_info_data_64_t info{};
    mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
    mach_port_t object = MACH_PORT_NULL;
    if (mach_vm_region(mach_task_self(), &start, &size, VM_REGION_BASIC_INFO_64, reinterpret_cast<vm_region_info_t>(&info), &count, &object) != KERN_SUCCESS) return false;
    if (object != MACH_PORT_NULL) mach_port_deallocate(mach_task_self(), object);
    region.begin = static_cast<std::uintptr_t>(start);
    region.end = static_cast<std::uintptr_t>(start + size);
    region.readable = (info.protection & VM_PROT_READ) != 0;
    region.writable = (info.protection & VM_PROT_WRITE) != 0;
    region.executable = (info.protection & VM_PROT_EXECUTE) != 0;
    return true;
}

inline bool Containing(const std::uintptr_t address, Region& region) {
    return FirstAtOrAbove(address, region) && region.begin <= address && address < region.end;
}

}

#endif

#endif
