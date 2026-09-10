#pragma once
#include <string>
#include <string_view>

namespace portpad {
// Strict scalar-value UTF-8. Cursor columns count code points, not bytes.
bool decodeUtf8(std::string_view input, std::u32string& output);
std::string encodeUtf8(std::u32string_view input);
}
