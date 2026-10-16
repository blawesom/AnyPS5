#include "SpirvBackend/SpirvLoopContinue.hpp"
#include <spirv/unified1/spirv.hpp>
#include <algorithm>
#include <stdexcept>

namespace ShaderRecompiler {

namespace {

struct SpirvInstruction {
    std::size_t offset;
    std::uint32_t count;
    std::uint32_t opcode;
};

struct SpirvBlock {
    std::uint32_t label;
    std::size_t first;
    std::size_t terminator;
    std::size_t function;
};

bool IsBlockTerminator(std::uint32_t opcode) {
    return opcode == spv::OpBranch || opcode == spv::OpBranchConditional || opcode == spv::OpSwitch || opcode == spv::OpReturn || opcode == spv::OpReturnValue || opcode == spv::OpKill || opcode == spv::OpUnreachable || opcode == spv::OpTerminateInvocation;
}

bool BranchTargetWord(const SpirvInstruction& terminator, std::uint32_t word) {
    if (terminator.opcode == spv::OpBranch) return word == 1;
    if (terminator.opcode == spv::OpBranchConditional) return word == 2 || word == 3;
    if (terminator.opcode == spv::OpSwitch) return word == 2 || (word >= 4 && word % 2 == 0);
    return false;
}

}

std::vector<std::uint32_t> SeparateLoopContinueTargets(std::span<const std::uint32_t> spirv) {
    std::vector<std::uint32_t> words(spirv.begin(), spirv.end());
    if (words.size() < 5 || words[0] != spv::MagicNumber) return words;
    std::vector<SpirvInstruction> instructions;
    for (std::size_t offset = 5; offset < words.size();) {
        const auto count = words[offset] >> spv::WordCountShift;
        if (count == 0 || count > words.size() - offset) throw std::runtime_error("SPIR-V loop continue separation: malformed instruction");
        instructions.push_back({offset, count, words[offset] & spv::OpCodeMask});
        offset += count;
    }
    const auto none = instructions.size();
    std::vector<SpirvBlock> blocks;
    std::size_t function = 0;
    for (std::size_t index = 0; index < instructions.size(); ++index) {
        const auto opcode = instructions[index].opcode;
        if (opcode == spv::OpFunction) ++function;
        if (opcode == spv::OpLabel) blocks.push_back({words[instructions[index].offset + 1], index, none, function});
        if (IsBlockTerminator(opcode) && !blocks.empty() && blocks.back().terminator == none) blocks.back().terminator = index;
    }
    struct Insertion {
        std::size_t offset;
        std::uint32_t label;
        std::uint32_t header;
    };
    std::vector<Insertion> insertions;
    auto bound = words[3];
    for (std::size_t header = 0; header < blocks.size(); ++header) {
        const auto& head = blocks[header];
        if (head.terminator == none || head.terminator <= head.first + 1) continue;
        const auto& merge = instructions[head.terminator - 1];
        if (merge.opcode != spv::OpLoopMerge || merge.count < 4) continue;
        const auto continueTarget = words[merge.offset + 2];
        if (continueTarget == head.label) continue;
        const auto continued = std::find_if(blocks.begin(), blocks.end(), [&](const SpirvBlock& block) { return block.label == continueTarget && block.function == head.function; });
        if (continued == blocks.end() || continued->terminator == none) continue;
        const auto& continuedEnd = instructions[continued->terminator];
        if (continued->terminator == continued->first + 1 && continuedEnd.opcode == spv::OpBranch && words[continuedEnd.offset + 1] == head.label) continue;
        const auto predecessors = [&](std::uint32_t label) {
            std::vector<std::size_t> found;
            for (std::size_t index = 0; index < blocks.size(); ++index) {
                if (blocks[index].function != head.function || blocks[index].terminator == none) continue;
                const auto& end = instructions[blocks[index].terminator];
                for (std::uint32_t word = 1; word < end.count; ++word) {
                    if (BranchTargetWord(end, word) && words[end.offset + word] == label) {
                        found.push_back(index);
                        break;
                    }
                }
            }
            return found;
        };
        const auto entering = predecessors(continueTarget);
        if (entering.size() != 1) continue;
        if (entering.front() != header) {
            const auto& entry = instructions[head.terminator];
            if (entry.opcode != spv::OpBranch || words[entry.offset + 1] != blocks[entering.front()].label || predecessors(blocks[entering.front()].label) != std::vector<std::size_t>{header}) continue;
        }
        std::vector<std::size_t> latches;
        bool switchLatch = false;
        for (auto candidate = header + 1; candidate < blocks.size() && blocks[candidate].function == head.function; ++candidate) {
            if (blocks[candidate].terminator == none) continue;
            const auto& end = instructions[blocks[candidate].terminator];
            bool targetsHeader = false;
            for (std::uint32_t word = 1; word < end.count; ++word) targetsHeader = targetsHeader || (BranchTargetWord(end, word) && words[end.offset + word] == head.label);
            if (!targetsHeader) continue;
            switchLatch = switchLatch || end.opcode == spv::OpSwitch;
            latches.push_back(candidate);
        }
        if (switchLatch || latches.size() != 1) continue;
        const auto& latch = blocks[latches.front()];
        const auto separated = bound++;
        const auto& end = instructions[latch.terminator];
        for (std::uint32_t word = 1; word < end.count; ++word) {
            if (BranchTargetWord(end, word) && words[end.offset + word] == head.label) words[end.offset + word] = separated;
        }
        words[merge.offset + 2] = separated;
        for (auto index = head.first + 1; index < head.terminator && instructions[index].opcode == spv::OpPhi; ++index) {
            const auto& phi = instructions[index];
            for (std::uint32_t word = 4; word < phi.count; word += 2) {
                if (words[phi.offset + word] == latch.label) words[phi.offset + word] = separated;
            }
        }
        insertions.push_back({end.offset + end.count, separated, head.label});
    }
    if (insertions.empty()) return words;
    words[3] = bound;
    std::stable_sort(insertions.begin(), insertions.end(), [](const Insertion& left, const Insertion& right) { return left.offset < right.offset; });
    std::vector<std::uint32_t> result;
    result.reserve(words.size() + insertions.size() * 4u);
    std::size_t copied = 0;
    for (const auto& insertion : insertions) {
        result.insert(result.end(), words.begin() + static_cast<std::ptrdiff_t>(copied), words.begin() + static_cast<std::ptrdiff_t>(insertion.offset));
        copied = insertion.offset;
        result.push_back((2u << spv::WordCountShift) | spv::OpLabel);
        result.push_back(insertion.label);
        result.push_back((2u << spv::WordCountShift) | spv::OpBranch);
        result.push_back(insertion.header);
    }
    result.insert(result.end(), words.begin() + static_cast<std::ptrdiff_t>(copied), words.end());
    return result;
}

}
