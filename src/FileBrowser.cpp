#include "FileBrowser.hpp"
#include <algorithm>

namespace portpad {
bool FileBrowser::open(const std::filesystem::path& path) {
    std::error_code error;
    auto absolute = std::filesystem::absolute(path, error).lexically_normal();
    if (absolute != absolute.root_path() && absolute.filename().empty()) absolute = absolute.parent_path();
    if (error) { message_ = "Cannot resolve path: " + error.message(); return false; }
    auto listing = listDirectory(absolute);
    if (!listing.error.empty()) { message_ = listing.error; return false; }
    path_ = absolute;
    entries_ = std::move(listing.entries);
    if (path_ != path_.root_path()) entries_.insert(entries_.begin(), {path_ / "..", true});
    selected_ = 0;
    message_.clear();
    return true;
}
void FileBrowser::handle(InputAction action) {
    if (entries_.empty()) return;
    if (action == InputAction::Up) selected_ = (selected_ + entries_.size() - 1) % entries_.size();
    else if (action == InputAction::Down) selected_ = (selected_ + 1) % entries_.size();
    else if (action == InputAction::PageUp) selected_ = selected_ > pageSize ? selected_ - pageSize : 0;
    else if (action == InputAction::PageDown) selected_ = std::min(selected_ + pageSize, entries_.size() - 1);
    else if (action == InputAction::Confirm) {
        const auto entry = entries_[selected_];
        if (entry.directory) {
            const auto child = path_.filename();
            const bool parent = entry.path.filename() == "..";
            if (open(entry.path) && parent) {
                for (std::size_t i = 0; i < entries_.size(); ++i)
                    if (entries_[i].path.filename() == child) { selected_ = i; break; }
            }
        }
        else message_ = "Read-only browser: file opening is not available yet.";
    }
}
}
