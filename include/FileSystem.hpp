#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace portpad {
struct FileEntry {
    std::filesystem::path path;
    bool directory;
};
struct DirectoryListing {
    std::vector<FileEntry> entries;
    std::string error;
};
DirectoryListing listDirectory(const std::filesystem::path& path);
}
