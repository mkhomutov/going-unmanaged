// Appendix F, Recipes 2-5, 17 and 23 - strings: split, join, build, format,
// the UTF-8 <-> UTF-16 boundary, and the empty string that is not null.
//
// The recipe functions below are quoted VERBATIM in book/F-rosetta-cookbook.md:
// editing one means editing the appendix in the same commit (the testlab
// discipline). main() is scaffolding, not part of any recipe - it asserts
// what the recipes claim, so build_all.sh keeps the cookbook honest.
#include <cassert>
#include <cstdio>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

// Recipe 2 - string.Split
std::vector<std::string> split(const std::string& text, char sep) {
    std::vector<std::string> parts;
    std::istringstream stream(text);
    std::string field;
    while (std::getline(stream, field, sep)) {
        parts.push_back(field);
    }
    return parts;
}

// Recipe 3 - string.Join
std::string join(const std::vector<std::string>& parts, const std::string& sep) {
    std::string result;
    for (const auto& part : parts) {
        if (!result.empty()) {
            result += sep;    // between elements only - never leading
        }
        result += part;
    }
    return result;
}

// Recipe 4 - StringBuilder
std::string build_report(const std::vector<int>& values) {
    std::string out;
    // one allocation up front - the StringBuilder(capacity) constructor
    out.reserve(values.size() * 12);
    for (int value : values) {
        out += "value=";
        out += std::to_string(value);
        out += '\n';
    }
    return out;
}

// Recipe 5 - string.Format
std::string describe(int count, double ratio) {
    std::ostringstream out;
    out << count << " samples, ratio "
        << std::fixed << std::setprecision(2) << ratio;
    return out.str();
}

std::string describe_c(int count, double ratio) {
    char buffer[64];
    std::snprintf(buffer, sizeof buffer, "%d samples, ratio %.2f", count, ratio);
    return buffer;
}

// Recipe 17 - the UTF-8 <-> UTF-16 boundary
// UTF-8 -> UTF-16. Invalid input becomes U+FFFD, the convention browsers
// follow; no exceptions, no locale, no deprecated machinery.
std::u16string utf8_to_utf16(std::string_view utf8) {
    std::u16string out;
    for (std::size_t i = 0; i < utf8.size(); ) {
        const auto b0 = static_cast<unsigned char>(utf8[i]);
        std::size_t n = b0 < 0x80          ? 1
                      : (b0 & 0xE0) == 0xC0 ? 2
                      : (b0 & 0xF0) == 0xE0 ? 3
                      : (b0 & 0xF8) == 0xF0 ? 4 : 0;
        char32_t cp = n == 1 ? b0
                    : n     ? b0 & (0x7Fu >> n)   // the lead byte's payload
                            : 0xFFFDu;            // stray or invalid lead
        std::size_t taken = 1;
        for (std::size_t k = 1; n && k < n && i + k < utf8.size(); ++k) {
            const auto bk = static_cast<unsigned char>(utf8[i + k]);
            if ((bk & 0xC0) != 0x80) { n = 0; break; }  // sequence cut short
            cp = (cp << 6) | (bk & 0x3Fu);
            ++taken;
        }
        if (n == 0 || taken != n || cp > 0x10FFFFu ||
            (cp >= 0xD800u && cp <= 0xDFFFu) ||           // surrogates
            (n == 2 && cp < 0x80u) || (n == 3 && cp < 0x800u) ||
            (n == 4 && cp < 0x10000u))                    // overlong forms
            cp = 0xFFFDu;
        i += taken;
        if (cp < 0x10000u) {
            out.push_back(static_cast<char16_t>(cp));
        } else {                                  // astral plane: a pair
            cp -= 0x10000u;
            out.push_back(static_cast<char16_t>(0xD800u + (cp >> 10)));
            out.push_back(static_cast<char16_t>(0xDC00u + (cp & 0x3FFu)));
        }
    }
    return out;
}

// UTF-16 -> UTF-8. Lone surrogates become U+FFFD; everything else is
// mechanical: split the code point across 1-4 bytes, high bits first.
std::string utf16_to_utf8(std::u16string_view utf16) {
    std::string out;
    for (std::size_t i = 0; i < utf16.size(); ++i) {
        char32_t cp = utf16[i];
        if (cp >= 0xD800u && cp <= 0xDBFFu && i + 1 < utf16.size() &&
            utf16[i + 1] >= 0xDC00u && utf16[i + 1] <= 0xDFFFu) {
            cp = 0x10000u + ((cp - 0xD800u) << 10) + (utf16[i + 1] - 0xDC00u);
            ++i;                                  // consumed the pair
        } else if (cp >= 0xD800u && cp <= 0xDFFFu) {
            cp = 0xFFFDu;                         // lone surrogate
        }
        if (cp < 0x80u) {
            out.push_back(static_cast<char>(cp));
        } else if (cp < 0x800u) {
            out.push_back(static_cast<char>(0xC0u | (cp >> 6)));
            out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
        } else if (cp < 0x10000u) {
            out.push_back(static_cast<char>(0xE0u | (cp >> 12)));
            out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
            out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
        } else {
            out.push_back(static_cast<char>(0xF0u | (cp >> 18)));
            out.push_back(static_cast<char>(0x80u | ((cp >> 12) & 0x3Fu)));
            out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
            out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
        }
    }
    return out;
}

// Recipe 23 - string.IsNullOrEmpty / s ?? ""
bool is_blank(const std::string& s) {
    return s.empty();                       // a std::string cannot be null: only empty
}

std::string name_or_default(const char* from_c_api) {
    if (from_c_api == nullptr) {            // the one null there is: a C API's "no name"
        return "unnamed";
    }
    return from_c_api;                      // safe now - std::string(nullptr) is UB
}

std::optional<std::string> label_of(bool has_label, const std::string& text) {
    if (!has_label) {
        return std::nullopt;                // "no string" is a different answer from ""
    }
    return text;                            // ...which may itself be empty, legitimately
}

int main() {
    // Recipe 2, including the two behaviors the appendix claims: interior
    // empty fields are kept, the final empty field is not (C# keeps it).
    assert((split("a,b,c", ',') == std::vector<std::string>{"a", "b", "c"}));
    assert((split("a,,b", ',') == std::vector<std::string>{"a", "", "b"}));
    assert((split("a,b,", ',') == std::vector<std::string>{"a", "b"}));

    // Recipe 3: no leading or trailing separator at any element count.
    assert(join({}, ", ").empty());
    assert(join({"x"}, ", ") == "x");
    assert(join({"x", "y", "z"}, ", ") == "x, y, z");

    // Recipe 4.
    assert(build_report({1, 2}) == "value=1\nvalue=2\n");
    assert(build_report({}).empty());

    // Recipe 5: both spellings produce the same text.
    assert(describe(3, 0.5) == "3 samples, ratio 0.50");
    assert(describe_c(3, 0.5) == "3 samples, ratio 0.50");

    // Recipe 17: the appendix's numbers, the damage policy, and - the real
    // oracle - every Unicode scalar value round-tripped both ways.
    {
        const std::string g = "Gr\xc3\xbc\xc3\x9f""e";          // "Grüße" spelled in bytes
        assert(g.size() == 7);
        assert(utf8_to_utf16(g).size() == 5);
        assert(utf16_to_utf8(utf8_to_utf16(g)) == g);
        const std::string clef = "\xf0\x9d\x84\x9e";            // U+1D11E, one code point
        assert(clef.size() == 4 && utf8_to_utf16(clef).size() == 2);
        assert(utf8_to_utf16("\xC3") == u"\uFFFD");               // cut short
        assert(utf8_to_utf16("\xC0\xAF") == u"\uFFFD");           // overlong form
        assert(utf16_to_utf8(std::u16string(1, char16_t(0xD800))) == "\xEF\xBF\xBD");
        for (char32_t cp = 0; cp <= 0x10FFFFu; ++cp) {
            if (cp >= 0xD800u && cp <= 0xDFFFu) continue;
            std::u16string u16;
            if (cp < 0x10000u) u16.push_back(static_cast<char16_t>(cp));
            else {
                const char32_t v = cp - 0x10000u;
                u16.push_back(static_cast<char16_t>(0xD800u + (v >> 10)));
                u16.push_back(static_cast<char16_t>(0xDC00u + (v & 0x3FFu)));
            }
            assert(utf8_to_utf16(utf16_to_utf8(u16)) == u16);
        }
    }

    // Recipe 23: empty is a value, null is a type - and the C pointer is the
    // only null. name_or_default takes the null; std::string never sees it.
    assert(is_blank(std::string()));
    assert(is_blank(""));
    assert(!is_blank(" "));
    const std::string s{};                  // {} and () and "" all mean the same empty
    assert(s == "" && s.size() == 0 && s.empty() && s.c_str()[0] == '\0');
    assert(name_or_default(nullptr) == "unnamed");
    assert(name_or_default("") == "");     // empty from the API is empty, not missing
    assert(name_or_default("sensor0") == "sensor0");
    assert(!label_of(false, "ignored").has_value());
    assert(label_of(true, "") == std::optional<std::string>{""});   // present and empty
    // The trap, as a comment: `std::string name = Thing_GetName(h);` with a
    // null return dies inside the constructor on libc++ and throws
    // std::logic_error on libstdc++ - scripts/check_platform_claims.sh
    // asserts both, per standard library.
    return 0;
}
