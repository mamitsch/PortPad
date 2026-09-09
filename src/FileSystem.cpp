#include "FileSystem.hpp"
#include <algorithm>

namespace portpad {
DirectoryListing listDirectory(const std::filesystem::path& path) {
    DirectoryListing result;
    std::error_code error;
    std::filesystem::directory_iterator iterator(path, error), end;
    while (!error && iterator != end) {
        std::error_code statusError;
        const bool directory = iterator->is_directory(statusError);
        // Broken links and entries with unavailable metadata remain visible as files.
        result.entries.push_back({iterator->path(), directory});
        iterator.increment(error);
    }
    if (error) {
        result.entries.clear();
        result.error = "Cannot list directory: " + error.message();
        return result;
    }
    std::sort(result.entries.begin(), result.entries.end(), [](const FileEntry& a, const FileEntry& b) {
        if (a.directory != b.directory) return a.directory;
        return a.path.filename().native() < b.path.filename().native();
    });
    return result;
}
}
