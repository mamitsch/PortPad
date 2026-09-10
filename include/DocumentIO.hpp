#pragma once
#include <filesystem>
#include <optional>
#include <string>
namespace portpad {
struct DocumentData {
    std::filesystem::path path;
    std::string text, original;
    bool crlf = false, bom = false;
};
class DocumentIO {
public:
    static bool supported(const std::filesystem::path& path);
    static bool load(const std::filesystem::path& path, DocumentData& result, std::string& error);
    static std::string serialize(const std::string& normalized, bool crlf, bool bom);
    // Capture an existing regular destination for overwrite confirmation/conflict checking.
    static bool inspect(const std::filesystem::path& path, std::optional<std::string>& original, std::string& error);
    // expected=nullopt means create-only. Existing files require an exact expected snapshot.
    static bool save(const std::filesystem::path& path, const std::string& bytes,
                     const std::optional<std::string>& expected, std::string& error);
};
}
