#pragma once
#include "InputAction.hpp"
#include <algorithm>
#include <string>
#include <vector>

namespace portpad {
// SDL-independent modal text viewport. Lines break only at UTF-8 boundaries.
class TextDialog {
public:
    static constexpr std::size_t visibleRows = 10;
    void open(std::string title, const std::string& text) {
        title_ = std::move(title);
        lines_.clear();
        std::string line;
        std::size_t columns = 0;
        for (std::size_t i = 0; i < text.size();) {
            if (text[i] == '\n') {
                lines_.push_back(line); line.clear(); columns = 0; ++i; continue;
            }
            if (columns == 16) { lines_.push_back(line); line.clear(); columns = 0; }
            auto end = i + 1;
            while (end < text.size() && (static_cast<unsigned char>(text[end]) & 0xc0) == 0x80) ++end;
            line.append(text, i, end - i);
            i = end;
            ++columns;
        }
        lines_.push_back(line);
        first_ = 0;
        active_ = true;
    }
    void handle(InputAction action) {
        if (action == InputAction::Back || action == InputAction::Confirm || action == InputAction::Menu) active_ = false;
        const auto last = lines_.size() > visibleRows ? lines_.size() - visibleRows : 0;
        if (action == InputAction::Down) first_ = std::min(first_ + 1, last);
        if (action == InputAction::Up && first_) --first_;
        if (action == InputAction::PageDown) first_ = std::min(first_ + visibleRows, last);
        if (action == InputAction::PageUp) first_ = first_ > visibleRows ? first_ - visibleRows : 0;
    }
    bool active() const { return active_; }
    const std::string& title() const { return title_; }
    const std::vector<std::string>& lines() const { return lines_; }
    std::size_t first() const { return first_; }
private:
    bool active_ = false;
    std::string title_;
    std::vector<std::string> lines_;
    std::size_t first_ = 0;
};
}
