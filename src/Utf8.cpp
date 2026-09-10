#include "Utf8.hpp"
namespace portpad {
bool decodeUtf8(std::string_view s, std::u32string& out) {
    std::u32string decoded;
    for (std::size_t i = 0; i < s.size();) {
        const auto c = static_cast<unsigned char>(s[i++]);
        char32_t value; unsigned extra; char32_t minimum;
        if (c < 0x80) { value = c; extra = 0; minimum = 0; }
        else if (c >= 0xc2 && c <= 0xdf) { value = c & 31; extra = 1; minimum = 0x80; }
        else if (c >= 0xe0 && c <= 0xef) { value = c & 15; extra = 2; minimum = 0x800; }
        else if (c >= 0xf0 && c <= 0xf4) { value = c & 7; extra = 3; minimum = 0x10000; }
        else return false;
        if (i + extra > s.size()) return false;
        for (unsigned j = 0; j < extra; ++j) {
            const auto next = static_cast<unsigned char>(s[i++]);
            if ((next & 0xc0) != 0x80) return false;
            value = (value << 6) | (next & 63);
        }
        if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff)) return false;
        decoded.push_back(value);
    }
    out = std::move(decoded);
    return true;
}
std::string encodeUtf8(std::u32string_view input) {
    std::string out;
    for (auto c : input) {
        if (c < 0x80) out.push_back(static_cast<char>(c));
        else if (c < 0x800) {
            out.push_back(static_cast<char>(0xc0 | (c >> 6)));
            out.push_back(static_cast<char>(0x80 | (c & 63)));
        } else if (c < 0x10000) {
            out.push_back(static_cast<char>(0xe0 | (c >> 12)));
            out.push_back(static_cast<char>(0x80 | ((c >> 6) & 63)));
            out.push_back(static_cast<char>(0x80 | (c & 63)));
        } else {
            out.push_back(static_cast<char>(0xf0 | (c >> 18)));
            out.push_back(static_cast<char>(0x80 | ((c >> 12) & 63)));
            out.push_back(static_cast<char>(0x80 | ((c >> 6) & 63)));
            out.push_back(static_cast<char>(0x80 | (c & 63)));
        }
    }
    return out;
}
}
