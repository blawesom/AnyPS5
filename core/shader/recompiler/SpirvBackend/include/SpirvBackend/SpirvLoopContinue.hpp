#ifndef CORE_SHADER_RECOMPILER_SPIRVBACKEND_SPIRVLOOPCONTINUE_HPP
#define CORE_SHADER_RECOMPILER_SPIRVBACKEND_SPIRVLOOPCONTINUE_HPP

#include <cstdint>
#include <span>
#include <vector>

namespace ShaderRecompiler {

[[nodiscard]] std::vector<std::uint32_t> SeparateLoopContinueTargets(std::span<const std::uint32_t> spirv);

}

#endif
