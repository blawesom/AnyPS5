#ifdef __APPLE__

#include "prx/libc/include/specifics/linux/ElfTypes.hpp"
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>
#include <mach-o/loader.h>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace {

constexpr std::uint64_t MetaVersion = 2;
constexpr std::size_t LoadBiasOffset = 0x08;
constexpr std::size_t HeaderCountOffset = 0x58;
constexpr std::size_t HeadersOffset = 0x60;

const std::vector<Elf64_Phdr>& SegmentHeaders(const mach_header_64* header) {
    static std::mutex lock;
    static auto* cache = new std::map<const mach_header_64*, std::vector<Elf64_Phdr>>();
    std::lock_guard guard(lock);
    auto [found, added] = cache->try_emplace(header);
    if (!added) return found->second;
    const auto* command = reinterpret_cast<const load_command*>(header + 1);
    std::uint64_t fileBase = 0;
    for (std::uint32_t index = 0; index < header->ncmds; ++index) {
        if (command->cmd == LC_SEGMENT_64) {
            const auto* segment = reinterpret_cast<const segment_command_64*>(command);
            if (segment->fileoff == 0 && segment->filesize != 0) fileBase = segment->vmaddr;
            if (segment->vmsize != 0 && segment->initprot != 0) {
                Elf64_Phdr phdr{};
                phdr.p_type = PT_LOAD;
                phdr.p_flags = ((segment->initprot & VM_PROT_READ) ? PF_R : 0u) | ((segment->initprot & VM_PROT_WRITE) ? PF_W : 0u) | ((segment->initprot & VM_PROT_EXECUTE) ? PF_X : 0u);
                phdr.p_offset = segment->filesize == 0 && segment->vmaddr > fileBase ? segment->vmaddr - fileBase : segment->fileoff;
                phdr.p_vaddr = segment->vmaddr;
                phdr.p_paddr = segment->vmaddr;
                phdr.p_filesz = segment->filesize;
                phdr.p_memsz = segment->vmsize;
                phdr.p_align = 0x4000;
                found->second.push_back(phdr);
            }
        }
        command = reinterpret_cast<const load_command*>(reinterpret_cast<const std::uint8_t*>(command) + command->cmdsize);
    }
    return found->second;
}

}

extern "C" int dl_iterate_phdr(int (*callback)(dl_phdr_info*, std::size_t, void*), void* data) {
    for (std::uint32_t index = 0; index < _dyld_image_count(); ++index) {
        const auto* header = reinterpret_cast<const mach_header_64*>(_dyld_get_image_header(index));
        if (header == nullptr || header->magic != MH_MAGIC_64 || (header->flags & MH_DYLIB_IN_CACHE) != 0) continue;
        unsigned long size = 0;
        const auto* meta = getsectiondata(header, "__APS5DATA", "__meta", &size);
        const char* imageName = _dyld_get_image_name(index);
        const bool hostModule = (header->filetype == MH_DYLIB || header->filetype == MH_BUNDLE) && imageName != nullptr;
        if ((meta == nullptr || size < HeadersOffset) && (header->filetype == MH_EXECUTE || hostModule)) {
            const auto& headers = SegmentHeaders(header);
            if (headers.empty()) continue;
            dl_phdr_info info {};
            info.dlpi_addr = static_cast<std::uintptr_t>(_dyld_get_image_vmaddr_slide(index));
            info.dlpi_name = header->filetype == MH_EXECUTE ? "" : imageName;
            info.dlpi_phdr = headers.data();
            info.dlpi_phnum = static_cast<std::uint16_t>(headers.size());
            if (const int result = callback(&info, sizeof(info), data); result != 0) return result;
            continue;
        }
        if (meta == nullptr || size < HeadersOffset) continue;
        std::uint64_t version;
        std::uint64_t loadBias;
        std::uint64_t count;
        std::memcpy(&version, meta, sizeof(version));
        std::memcpy(&loadBias, meta + LoadBiasOffset, sizeof(loadBias));
        std::memcpy(&count, meta + HeaderCountOffset, sizeof(count));
        if (version != MetaVersion || count > 0xffff || count > (size - HeadersOffset) / sizeof(Elf64_Phdr)) continue;
        dl_phdr_info info {};
        info.dlpi_addr = static_cast<std::uintptr_t>(loadBias);
        info.dlpi_name = header->filetype == MH_EXECUTE ? "" : _dyld_get_image_name(index);
        info.dlpi_phdr = reinterpret_cast<const Elf64_Phdr*>(meta + HeadersOffset);
        info.dlpi_phnum = static_cast<std::uint16_t>(count);
        if (const int result = callback(&info, sizeof(info), data); result != 0) return result;
    }
    return 0;
}

namespace {

struct ImageEntry {
    const mach_header_64* header = nullptr;
    std::uint32_t index = 0;
};

bool ImageOf(const void* base, ImageEntry& entry) {
    for (std::uint32_t index = 0; index < _dyld_image_count(); ++index) {
        if (_dyld_get_image_header(index) == base) {
            entry = {reinterpret_cast<const mach_header_64*>(base), index};
            return true;
        }
    }
    return false;
}

std::uintptr_t LoadBias(const ImageEntry& entry) {
    unsigned long size = 0;
    const auto* meta = getsectiondata(entry.header, "__APS5DATA", "__meta", &size);
    if (meta != nullptr && size >= HeadersOffset) {
        std::uint64_t loadBias = 0;
        std::memcpy(&loadBias, meta + LoadBiasOffset, sizeof(loadBias));
        return static_cast<std::uintptr_t>(loadBias);
    }
    return static_cast<std::uintptr_t>(_dyld_get_image_vmaddr_slide(entry.index));
}

link_map* MapFor(const ImageEntry& entry) {
    static std::mutex lock;
    static auto* maps = new std::map<const mach_header_64*, std::unique_ptr<link_map>>();
    std::lock_guard guard(lock);
    auto& map = (*maps)[entry.header];
    if (!map) {
        map = std::make_unique<link_map>();
        map->l_addr = LoadBias(entry);
        const char* name = entry.header->filetype == MH_EXECUTE ? "" : _dyld_get_image_name(entry.index);
        map->l_name = strdup(name != nullptr ? name : "");
    }
    return map.get();
}

}

extern "C" int dladdr1(const void* address, Dl_info* info, void** extra, int flags) {
    if (info == nullptr || extra == nullptr || flags != RTLD_DL_LINKMAP) return 0;
    Dl_info found{};
    if (dladdr(address, &found) == 0 || found.dli_fbase == nullptr) return 0;
    ImageEntry entry;
    if (!ImageOf(found.dli_fbase, entry)) return 0;
    *info = found;
    *extra = MapFor(entry);
    return 1;
}

extern "C" int dlinfo(void* handle, int request, void* output) {
    if (handle == nullptr || output == nullptr || request != RTLD_DI_LINKMAP) return -1;
    void* main = dlopen(nullptr, RTLD_LAZY | RTLD_NOLOAD);
    const bool wantsMain = main == handle;
    if (main != nullptr) dlclose(main);
    for (std::uint32_t index = 0; index < _dyld_image_count(); ++index) {
        const auto* header = reinterpret_cast<const mach_header_64*>(_dyld_get_image_header(index));
        if (header == nullptr) continue;
        bool matches = false;
        if (wantsMain) {
            matches = header->filetype == MH_EXECUTE;
        } else if (const char* name = _dyld_get_image_name(index); name != nullptr) {
            void* opened = dlopen(name, RTLD_LAZY | RTLD_NOLOAD);
            matches = opened == handle;
            if (opened != nullptr) dlclose(opened);
        }
        if (matches) {
            *static_cast<link_map**>(output) = MapFor({header, index});
            return 0;
        }
    }
    return -1;
}

#endif
