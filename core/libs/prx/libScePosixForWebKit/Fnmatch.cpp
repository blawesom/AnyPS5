#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>

extern "C" {
std::size_t APS5_VABI mbrtowc_nid_postfix(std::uint16_t*, const char*, std::size_t, void*);
std::uint32_t APS5_VABI towlower_nid_postfix(std::uint32_t);
}

namespace {
constexpr int NoMatch = 1;
constexpr int NoEscape = 0x01;
constexpr int Pathname = 0x02;
constexpr int Period = 0x04;
constexpr int LeadingDir = 0x08;
constexpr int CaseFold = 0x10;
constexpr std::size_t MultibyteLimit = 6;
constexpr auto Invalid = static_cast<std::size_t>(-1);
constexpr auto Incomplete = static_cast<std::size_t>(-2);

struct State {
    alignas(8) unsigned char bytes[128];
};

enum class Bracket { Match, NoMatch, Error };

bool Failed(std::size_t length) {
    return length == Invalid || length == Incomplete;
}

std::size_t Decode(std::uint32_t& character, const char* text, State& state) {
    std::uint16_t value = 0;
    const auto length = mbrtowc_nid_postfix(&value, text, MultibyteLimit, state.bytes);
    character = value;
    return length;
}

std::size_t DecodeString(std::uint32_t& character, const char* text, State& state) {
    const auto length = Decode(character, text, state);
    if (!Failed(length)) return length;
    character = static_cast<unsigned char>(*text);
    state = {};
    return 1;
}

bool LeadingPeriod(std::uint32_t character, const char* string, const char* start, int flags) {
    return character == '.' && (flags & Period) && (string == start || ((flags & Pathname) && string[-1] == '/'));
}

bool Same(std::uint32_t patternCharacter, std::uint32_t stringCharacter, int flags) {
    return patternCharacter == stringCharacter ||
        ((flags & CaseFold) && towlower_nid_postfix(patternCharacter) == towlower_nid_postfix(stringCharacter));
}

Bracket MatchBracket(const char* pattern, std::uint32_t test, int flags, const char*& next, State& state) {
    const bool negate = *pattern == '!' || *pattern == '^';
    if (negate) ++pattern;
    if (flags & CaseFold) test = towlower_nid_postfix(test);
    bool matched = false;
    const char* const first = pattern;
    for (;;) {
        if (*pattern == ']' && pattern > first) {
            ++pattern;
            break;
        }
        if (*pattern == '\0') return Bracket::Error;
        if (*pattern == '/' && (flags & Pathname)) return Bracket::NoMatch;
        if (*pattern == '\\' && !(flags & NoEscape)) ++pattern;
        std::uint32_t low = 0;
        auto length = Decode(low, pattern, state);
        if (Failed(length)) return Bracket::NoMatch;
        pattern += length;
        if (flags & CaseFold) low = towlower_nid_postfix(low);
        if (*pattern == '-' && pattern[1] != '\0' && pattern[1] != ']') {
            if (*++pattern == '\\' && !(flags & NoEscape)) ++pattern;
            std::uint32_t high = 0;
            length = Decode(high, pattern, state);
            if (Failed(length)) return Bracket::NoMatch;
            pattern += length;
            if (high == '\0') return Bracket::Error;
            if (flags & CaseFold) high = towlower_nid_postfix(high);
            if (low <= test && test <= high) matched = true;
        } else if (low == test) {
            matched = true;
        }
    }
    next = pattern;
    return matched == negate ? Bracket::NoMatch : Bracket::Match;
}

int Match(const char* pattern, const char* string, int flags) {
    const char* const start = string;
    State patternState{};
    State stringState{};
    const char* backtrackPattern = nullptr;
    const char* backtrackString = nullptr;
    State backtrackPatternState{};
    State backtrackStringState{};
    for (;;) {
        std::uint32_t patternCharacter = 0;
        auto patternLength = Decode(patternCharacter, pattern, patternState);
        if (Failed(patternLength)) return NoMatch;
        pattern += patternLength;
        std::uint32_t stringCharacter = 0;
        const auto stringLength = DecodeString(stringCharacter, string, stringState);
        bool mismatch = false;
        switch (patternCharacter) {
        case '\0':
            if ((flags & LeadingDir) && stringCharacter == '/') return 0;
            if (stringCharacter == '\0') return 0;
            mismatch = true;
            break;
        case '?':
            if (stringCharacter == '\0') return NoMatch;
            if ((stringCharacter == '/' && (flags & Pathname)) || LeadingPeriod(stringCharacter, string, start, flags)) {
                mismatch = true;
                break;
            }
            string += stringLength;
            break;
        case '*': {
            auto next = *pattern;
            while (next == '*') next = *++pattern;
            if (LeadingPeriod(stringCharacter, string, start, flags)) {
                mismatch = true;
                break;
            }
            if (next == '\0') return (flags & Pathname) && !(flags & LeadingDir) && std::strchr(string, '/') ? NoMatch : 0;
            if (next == '/' && (flags & Pathname)) {
                string = std::strchr(string, '/');
                if (string == nullptr) return NoMatch;
                break;
            }
            backtrackPattern = pattern;
            backtrackPatternState = patternState;
            backtrackString = string;
            backtrackStringState = stringState;
            break;
        }
        case '[': {
            if (stringCharacter == '\0') return NoMatch;
            if ((stringCharacter == '/' && (flags & Pathname)) || LeadingPeriod(stringCharacter, string, start, flags)) {
                mismatch = true;
                break;
            }
            const char* next = nullptr;
            const auto bracket = MatchBracket(pattern, stringCharacter, flags, next, patternState);
            if (bracket == Bracket::NoMatch) {
                mismatch = true;
                break;
            }
            if (bracket == Bracket::Match) pattern = next;
            string += stringLength;
            if (bracket == Bracket::Error) mismatch = !Same(patternCharacter, stringCharacter, flags);
            break;
        }
        case '\\':
            if (!(flags & NoEscape)) {
                patternLength = Decode(patternCharacter, pattern, patternState);
                if (patternLength == 0 || Failed(patternLength)) return NoMatch;
                pattern += patternLength;
            }
            [[fallthrough]];
        default:
            string += stringLength;
            mismatch = !Same(patternCharacter, stringCharacter, flags);
            break;
        }
        if (!mismatch) continue;
        if (backtrackPattern == nullptr) return NoMatch;
        std::uint32_t skipped = 0;
        const auto skippedLength = DecodeString(skipped, backtrackString, backtrackStringState);
        if (skipped == '\0') return NoMatch;
        if (skipped == '/' && (flags & Pathname)) return NoMatch;
        backtrackString += skippedLength;
        pattern = backtrackPattern;
        patternState = backtrackPatternState;
        string = backtrackString;
        stringState = backtrackStringState;
    }
}
}

extern "C" int APS5_VABI fnmatch_nid_postfix(const char* pattern, const char* string, int flags) {
    if (pattern == nullptr || string == nullptr) throw std::invalid_argument("fnmatch: null pattern or string");
    return Match(pattern, string, flags);
}
