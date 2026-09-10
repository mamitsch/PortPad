#pragma once
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace portpad {
class TextBuffer {
public:
    static constexpr std::size_t maxBytes = 1024 * 1024;
    bool load(std::string_view normalizedUtf8);
    bool insert(std::string_view utf8);
    bool backspace();
    bool erase();
    bool undo();
    bool redo();
    bool canUndo() const { return !undo_.empty(); }
    bool canRedo() const { return !redo_.empty(); }
    void moveHorizontal(int direction);
    void moveVertical(int lines);
    bool goToLine(std::string_view oneBased);
    void setCursor(std::size_t offset);
    std::optional<std::size_t> find(std::string_view query, std::size_t from) const;
    std::string utf8() const;
    const std::u32string& text() const { return text_; }
    std::size_t cursor() const { return cursor_; }
    std::size_t line() const;
    std::size_t column() const;
    std::size_t lineCount() const { return starts_.size(); }
    std::size_t lineStart(std::size_t row) const { return starts_.at(row); }
    std::u32string_view lineText(std::size_t row) const;
    std::size_t displayColumn() const;
    bool dirty() const { return text_ != saved_; }
    void markSaved() { saved_ = text_; }
private:
    struct Snapshot { std::u32string text; std::size_t cursor; };
    std::u32string text_, saved_;
    std::size_t cursor_ = 0, preferred_ = 0;
    std::vector<std::size_t> starts_{0};
    std::deque<Snapshot> undo_, redo_;
    void rebuild();
    void remember(std::deque<Snapshot>& history);
    bool replace(std::size_t at, std::size_t count, std::u32string_view value);
};
}
