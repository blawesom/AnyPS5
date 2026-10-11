#include "prx/libc/include/general/VabiMacros.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" int APS5_VABI fnmatch_nid_postfix(const char*, const char*, int);

namespace {
constexpr int NoMatch = 1;
constexpr int NoEscape = 0x01;
constexpr int Pathname = 0x02;
constexpr int Period = 0x04;
constexpr int LeadingDir = 0x08;
constexpr int CaseFold = 0x10;

struct Case {
    const char* pattern;
    const char* string;
    int flags;
    int result;
};

constexpr Case FreeBsdCases[] = {
    {"", "", 0, 0},
    {"a", "a", 0, 0},
    {"a", "b", 0, NoMatch},
    {"a", "A", 0, NoMatch},
    {"*", "a", 0, 0},
    {"*", "aa", 0, 0},
    {"*a", "b", 0, NoMatch},
    {"*a*b*", "qaqbq", 0, 0},
    {"*a*bc*", "qaqbqbcq", 0, 0},
    {"*a*bb", "qaqbqbb", 0, 0},
    {"*a*bc", "qaqbqbcq", 0, NoMatch},
    {"*a*a*a*a*a*a*a*a*a*a*", "aaaaaaaaa", 0, NoMatch},
    {"*a*a*a*a*a*a*a*a*a*a*", "aaaaaaaaaa", 0, 0},
    {".*.*.*.*.*.*.*.*.*.*", ".........", 0, NoMatch},
    {".*.*.*.*.*.*.*.*.*.*", "...........", 0, 0},
    {"*?*?*?*?*?*?*?*?*?*?*", "123456789", 0, NoMatch},
    {"??????????*", "1234567890", 0, 0},
    {"*??????????", "12345678901", 0, 0},
    {"[x]", "x", 0, 0},
    {"[*]", "*", 0, 0},
    {"[?]", "?", 0, 0},
    {"[", "[", 0, 0},
    {"[[]", "[", 0, 0},
    {"[[]", "x", 0, NoMatch},
    {"[*]", "", 0, NoMatch},
    {"[?]", "x", 0, NoMatch},
    {"*[*]*", "foo*foo", 0, 0},
    {"*[*]*", "foo", 0, NoMatch},
    {"[0-9]", "0", 0, 0},
    {"[0-9]", "9", 0, 0},
    {"[0-9]", "/", 0, NoMatch},
    {"[0-9]", ":", 0, NoMatch},
    {"[!0-9]", "5", 0, NoMatch},
    {"[!0-9]", "/", 0, 0},
    {"[!0-9]", "*", 0, 0},
    {"*[0-9]", "a5", 0, 0},
    {"*[0-9]", "a:", 0, NoMatch},
    {"*[!0-9]", "a9", 0, NoMatch},
    {"*[!0-9]", "a/", 0, 0},
    {"*[0-9]", "a9a9", 0, 0},
    {"\\*", "*", 0, 0},
    {"\\?", "?", 0, 0},
    {"\\[x]", "[x]", 0, 0},
    {"\\\\", "\\", 0, 0},
    {"*\\**", "foo*foo", 0, 0},
    {"*\\**", "foo", 0, NoMatch},
    {"*\\\\*", "foo\\foo", 0, 0},
    {"\\a", "a", 0, 0},
    {"\\*", "a", 0, NoMatch},
    {"\\*", "\\*", 0, NoMatch},
    {"\\[x]", "\\x", 0, NoMatch},
    {"\\a", "\\a", 0, NoMatch},
    {"\\", "\\", 0, NoMatch},
    {"\\", "", 0, NoMatch},
    {"\\*", "\\*", NoEscape, 0},
    {"\\", "\\", NoEscape, 0},
    {"\\\\", "\\", NoEscape, NoMatch},
    {"\\\\", "\\\\", NoEscape, 0},
    {"*\\*", "foo\\foo", NoEscape, 0},
    {"*\\*", "foo", NoEscape, NoMatch},
    {"*", ".", Period, NoMatch},
    {"?", ".", Period, NoMatch},
    {".*", ".", 0, 0},
    {".*", ".a", 0, 0},
    {"[0-9]", ".", Period, NoMatch},
    {"a*", "a.", 0, 0},
    {"a/a", "a/a", Pathname, 0},
    {"*/a", "a/a", Pathname, 0},
    {"*/*", "a/a", Pathname, 0},
    {"a*b/*", "abbb/.x", Pathname, 0},
    {"*", "a/a", Pathname, NoMatch},
    {"*/*", "a/a/a", Pathname, NoMatch},
    {"b/*", "b/.x", Pathname | Period, NoMatch},
    {"b*/*", "a/.x", Pathname | Period, NoMatch},
    {"b/.*", "b/.x", Pathname | Period, 0},
    {"b*/.*", "b/.x", Pathname | Period, 0},
    {"a", "A", CaseFold, 0},
    {"A", "a", CaseFold, 0},
    {"[a]", "A", CaseFold, 0},
    {"[A]", "a", CaseFold, 0},
    {"a", "b", CaseFold, NoMatch},
    {"a", "a/b", Pathname, NoMatch},
    {"*b", "a/b", Pathname, NoMatch},
    {"a", "a/b", Pathname | LeadingDir, 0},
    {"*", "a/b", Pathname | LeadingDir, 0},
    {"*", ".a/b", Pathname | LeadingDir, 0},
    {"*a", ".a/b", Pathname | LeadingDir, 0},
    {"*", ".a/b", Pathname | Period | LeadingDir, NoMatch},
    {"*a", ".a/b", Pathname | Period | LeadingDir, NoMatch},
    {"a*b/*", "abbb/.x", Pathname | Period, NoMatch},
};

constexpr Case BracketCases[] = {
    {"[]]", "]", 0, 0},
    {"[]a]", "a", 0, 0},
    {"[!]]", "]", 0, NoMatch},
    {"[!]]", "a", 0, 0},
    {"[^a]", "b", 0, 0},
    {"[^a]", "a", 0, NoMatch},
    {"[a-]", "-", 0, 0},
    {"[\\]]", "]", 0, 0},
    {"[\\!a]", "!", 0, 0},
    {"[a\\-z]", "-", 0, 0},
    {"[a\\-z]", "b", 0, NoMatch},
    {"[a-\\z]", "m", 0, 0},
    {"[a-\\z]", "m", NoEscape, NoMatch},
    {"[\\a]", "\\", NoEscape, 0},
    {"a[/]b", "a/b", 0, 0},
    {"a[/]b", "a/b", Pathname, NoMatch},
    {"[a/b]", "a", Pathname, NoMatch},
    {"[a", "[a", 0, 0},
    {"[a-", "[a-", 0, 0},
    {"[a-z", "m", 0, NoMatch},
    {"[A-Z]", "m", CaseFold, 0},
    {"[a-z]", "M", CaseFold, 0},
    {"[a-z]", "M", 0, NoMatch},
    {"[z-a]", "m", 0, NoMatch},
    {"\xc9", "\xe9", CaseFold, NoMatch},
    {"[\xe0-\xef]", "\xe9", 0, 0},
    {"[\xe0-\xef]", "\xf0", 0, NoMatch},
    {"?", "\xff", 0, 0},
    {"*\\", "a\\", 0, NoMatch},
    {"a/b", "a/b/c", LeadingDir, 0},
    {"a/b", "a/bc", LeadingDir, NoMatch},
    {"a*", "a/b", Pathname | LeadingDir, 0},
    {"*/b", "a/b/c", Pathname | LeadingDir, 0},
    {".a", ".a", Period, 0},
    {"a/?", "a/.", Pathname | Period, NoMatch},
    {"a/?", "a/.", Period, 0},
    {"a/[.]", "a/.", Pathname | Period, NoMatch},
    {"*", "a/.b", Period, 0},
    {"a", "a", 0x20, 0},
};

int failures = 0;

void Check(const Case& entry, int flags) {
    const int result = fnmatch_nid_postfix(entry.pattern, entry.string, flags);
    if (result == entry.result) return;
    std::fprintf(stderr, "fnmatch(\"%s\", \"%s\", 0x%x) returned %d, expected %d\n", entry.pattern, entry.string, flags,
        result, entry.result);
    ++failures;
}

void CheckFreeBsd(const Case& entry) {
    const bool patternEscapes = std::strchr(entry.pattern, '\\') != nullptr;
    const bool stringEscapes = std::strchr(entry.string, '\\') != nullptr;
    const bool stringSlash = std::strchr(entry.string, '/') != nullptr;
    const bool hiddenString = entry.string[0] == '.' && entry.pattern[0] != '.';
    const bool noMatch = entry.result == NoMatch;
    const int flags = entry.flags;
    Check(entry, flags);
    if (!patternEscapes && !(flags & NoEscape)) Check(entry, flags | NoEscape);
    if (patternEscapes && !stringEscapes && noMatch && !(flags & (NoEscape | LeadingDir))) Check(entry, flags | NoEscape);
    if ((!hiddenString || noMatch) && !(flags & (Pathname | Period))) Check(entry, flags | Period);
    if ((!stringSlash || noMatch) && !(flags & Pathname)) Check(entry, flags | Pathname);
    if (((!hiddenString && std::strstr(entry.string, "/.") == nullptr) || noMatch) && (flags & Pathname) &&
        !(flags & Period))
        Check(entry, flags | Period);
    if (((!hiddenString && !stringSlash) || noMatch) && !(flags & (Pathname | Period))) Check(entry, flags | Pathname | Period);
    if ((!stringSlash || !noMatch) && !(flags & LeadingDir)) Check(entry, flags | LeadingDir);
    if (!noMatch && !(flags & CaseFold)) Check(entry, flags | CaseFold);
    if (!patternEscapes && !noMatch && !(flags & (NoEscape | CaseFold))) Check(entry, flags | NoEscape | CaseFold);
}
}

int main() {
    for (const auto& entry : FreeBsdCases) CheckFreeBsd(entry);
    for (const auto& entry : BracketCases) Check(entry, entry.flags);
    return failures == 0 ? 0 : 1;
}
