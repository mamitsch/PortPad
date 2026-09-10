#include "DocumentIO.hpp"
#include "TextBuffer.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace portpad {
namespace {
std::string systemError(const char* action) { return std::string(action) + ": " + std::strerror(errno); }
struct File {
    int fd = -1;
    explicit File(int value) : fd(value) {}
    ~File() { if (fd >= 0) ::close(fd); }
    bool close(std::string& error) {
        const auto value = fd; fd = -1;
        if (::close(value) != 0) { error = systemError("Close failed"); return false; }
        return true;
    }
};
bool readRegular(const std::filesystem::path& path, std::string& bytes, struct stat& info, std::string& error) {
    File file(::open(path.c_str(), O_RDONLY | O_NONBLOCK | O_NOFOLLOW | O_CLOEXEC));
    if (file.fd < 0) { error = systemError("Cannot open file"); return false; }
    if (::fstat(file.fd, &info) != 0) { error = systemError("Cannot inspect file"); return false; }
    if (!S_ISREG(info.st_mode)) { error = "Only regular text files can be edited."; return false; }
    if (info.st_size > static_cast<off_t>(TextBuffer::maxBytes)) { error = "File exceeds the 1 MiB editor limit."; return false; }
    bytes.clear(); std::array<char, 8192> block;
    for (;;) {
        auto count = ::read(file.fd, block.data(), block.size());
        if (count < 0 && errno == EINTR) continue;
        if (count < 0) { error = systemError("Read failed"); return false; }
        if (!count) break;
        bytes.append(block.data(), static_cast<std::size_t>(count));
        if (bytes.size() > TextBuffer::maxBytes) { error = "File exceeds the 1 MiB editor limit."; return false; }
    }
    return file.close(error);
}
struct Temporary {
    std::filesystem::path path;
    ~Temporary() { if (!path.empty()) ::unlink(path.c_str()); }
};
bool prepare(const std::filesystem::path& directory, const std::string& bytes, mode_t mode,
             Temporary& temporary, std::string& error) {
    auto pattern = (directory / ".portpad-XXXXXX").string();
    File file(::mkstemp(pattern.data()));
    if (file.fd < 0) { error = systemError("Cannot create temporary file"); return false; }
    temporary.path = pattern;
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        auto count = ::write(file.fd, bytes.data() + offset, bytes.size() - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { error = systemError("Write failed"); return false; }
        offset += static_cast<std::size_t>(count);
    }
    if (::fchmod(file.fd, mode & 0777) != 0) {
        // Some handheld storage exposes fixed mount permissions (FAT/exFAT).
        // Preserve Unix mode bits where supported; still require a flushed file.
        if (errno != EPERM && errno != EOPNOTSUPP && errno != ENOSYS) {
            error = systemError("Cannot set temporary file permissions"); return false;
        }
    }
    if (::fsync(file.fd) != 0) { error = systemError("Cannot flush temporary file"); return false; }
    return file.close(error);
}
}
bool DocumentIO::supported(const std::filesystem::path& path) {
    auto extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (const auto* allowed : {".txt", ".sh", ".cfg", ".ini", ".gptk", ".json", ".xml", ".md"})
        if (extension == allowed) return true;
    return false;
}
bool DocumentIO::load(const std::filesystem::path& path, DocumentData& result, std::string& error) {
    error.clear();
    if (!supported(path)) { error = "Unsupported extension. Choose txt, sh, cfg, ini, gptk, json, xml or md."; return false; }
    DocumentData candidate; std::error_code ec;
    candidate.path = std::filesystem::canonical(path, ec);
    if (ec) { error = "Cannot resolve file: " + ec.message(); return false; }
    struct stat info{};
    if (!readRegular(candidate.path, candidate.original, info, error)) return false;
    candidate.bom = candidate.original.compare(0, 3, "\xef\xbb\xbf") == 0;
    const auto start = candidate.bom ? 3u : 0u;
    bool lf = false;
    for (std::size_t i = start; i < candidate.original.size(); ++i) {
        if (candidate.original[i] == '\r') {
            if (i + 1 == candidate.original.size() || candidate.original[i + 1] != '\n') {
                error = "Unsupported bare CR line ending."; return false;
            }
            candidate.crlf = true; ++i; candidate.text += '\n';
        } else {
            if (candidate.original[i] == '\n') lf = true;
            candidate.text += candidate.original[i];
        }
    }
    if (lf && candidate.crlf) { error = "Mixed LF/CRLF endings are not supported; file was not changed."; return false; }
    TextBuffer check;
    if (!check.load(candidate.text)) { error = "Not valid UTF-8 text (binary/control bytes or invalid encoding)."; return false; }
    result = std::move(candidate); return true;
}
std::string DocumentIO::serialize(const std::string& text, bool crlf, bool bom) {
    std::string bytes = bom ? "\xef\xbb\xbf" : "";
    for (char c : text) { if (crlf && c == '\n') bytes += '\r'; bytes += c; }
    return bytes;
}
bool DocumentIO::inspect(const std::filesystem::path& path, std::optional<std::string>& original, std::string& error) {
    original.reset(); error.clear(); struct stat info{};
    if (::lstat(path.c_str(), &info) != 0) {
        if (errno == ENOENT) return true;
        error = systemError("Cannot inspect destination"); return false;
    }
    std::string bytes;
    if (!readRegular(path, bytes, info, error)) return false;
    original = std::move(bytes); return true;
}
bool DocumentIO::save(const std::filesystem::path& path, const std::string& bytes,
                      const std::optional<std::string>& expected, std::string& error) {
    error.clear();
    if (!supported(path)) { error = "Save As requires a supported text extension (not .bak)."; return false; }
    if (bytes.size() > TextBuffer::maxBytes) { error = "Encoded file exceeds 1 MiB."; return false; }
    struct stat info{}; std::string old;
    const bool exists = ::lstat(path.c_str(), &info) == 0;
    if (!exists && errno != ENOENT) { error = systemError("Cannot inspect destination"); return false; }
    if (exists) {
        if (!readRegular(path, old, info, error)) return false;
        if (!expected || old != *expected) { error = "Destination changed since it was opened/confirmed. Use Save As or reload."; return false; }
        if (!(info.st_mode & 0222) || info.st_nlink > 1) { error = "Read-only or hard-linked destination; use Save As."; return false; }
    } else if (expected) { error = "Original file disappeared. Use Save As."; return false; }
    auto directory = path.parent_path(); if (directory.empty()) directory = ".";
    Temporary replacement;
    if (!prepare(directory, bytes, exists ? info.st_mode : 0600, replacement, error)) return false;
    if (exists) {
        const auto backup = std::filesystem::path(path.string() + ".bak");
        struct stat backupInfo{};
        if (::lstat(backup.c_str(), &backupInfo) == 0) {
            if (!S_ISREG(backupInfo.st_mode)) { error = "Backup path is not a regular file; save cancelled."; return false; }
        } else if (errno != ENOENT) { error = systemError("Cannot inspect backup"); return false; }
        Temporary backupTemp;
        if (!prepare(directory, old, info.st_mode, backupTemp, error)) return false;
        std::string current; struct stat currentInfo{};
        if (!readRegular(path, current, currentInfo, error)) return false;
        if (current != old || currentInfo.st_ino != info.st_ino || currentInfo.st_dev != info.st_dev) {
            error = "File changed during save; destination was not replaced."; return false;
        }
        if (::rename(backupTemp.path.c_str(), backup.c_str()) != 0) { error = systemError("Cannot replace backup"); return false; }
        if (::rename(replacement.path.c_str(), path.c_str()) != 0) { error = systemError("Cannot replace original"); return false; }
    } else {
        // Use the kernel interface to retain the firmware's older glibc baseline.
        // RENAME_NOREPLACE works on modern FAT as well as Unix filesystems.
        if (::syscall(SYS_renameat2, AT_FDCWD, replacement.path.c_str(), AT_FDCWD, path.c_str(), 1u) != 0) {
            const auto failure = errno;
            if (failure != ENOSYS && failure != EINVAL && failure != EOPNOTSUPP) {
                error = systemError("Cannot create destination"); return false;
            }
            // Older kernels may still support atomic create via a hard link.
            if (::link(replacement.path.c_str(), path.c_str()) != 0) {
                error = "Storage does not support safe file creation: " + std::string(std::strerror(errno)); return false;
            }
        }
    }
    File dir(::open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC));
    if (dir.fd < 0 || ::fsync(dir.fd) != 0) { error = "File written, but directory flush failed. Keep this buffer and check the storage."; return false; }
    return dir.close(error);
}
}
