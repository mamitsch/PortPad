#pragma once
#include "FileSystem.hpp"
#include "InputAction.hpp"

namespace portpad {
class FileBrowser {
public:
    static constexpr std::size_t pageSize = 6;
    bool open(const std::filesystem::path& path);
    void handle(InputAction action);
    const std::filesystem::path& path() const { return path_; }
    const std::vector<FileEntry>& entries() const { return entries_; }
    const std::string& message() const { return message_; }
    std::size_t selected() const { return selected_; }
private:
    std::filesystem::path path_;
    std::vector<FileEntry> entries_;
    std::size_t selected_ = 0;
    std::string message_;
};
}
