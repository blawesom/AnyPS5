#ifndef CORE_SHADER_RECOMPILER_SPIRVFMA_HPP
#define CORE_SHADER_RECOMPILER_SPIRVFMA_HPP

#include <spirv/unified1/spirv.hpp>

namespace ShaderRecompiler {

// TODO(technical debt): 3rdparty/SPIRV-Headers predates SPV_KHR_fma, so its opcode and capability
// are spelled out here. Use spv::OpFmaKHR and spv::CapabilityFMAKHR once the submodule has them.
inline constexpr auto SpirvOpFmaKHR = static_cast<spv::Op>(4427);
inline constexpr auto SpirvCapabilityFmaKHR = static_cast<spv::Capability>(6030);
inline constexpr const char* SpirvFmaExtension = "SPV_KHR_fma";

}

#endif
