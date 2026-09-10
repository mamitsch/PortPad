#include "TextBuffer.hpp"
#include "Utf8.hpp"
#include <algorithm>
#include <charconv>

namespace portpad {
namespace {
bool editable(std::u32string_view text) {
    for (auto c : text)
        if ((c < 32 && c != U'\n' && c != U'\t') || (c >= 127 && c < 160)) return false;
    return true;
}
}
bool TextBuffer::load(std::string_view utf8) {
    std::u32string candidate;
    if (utf8.size() > maxBytes || !decodeUtf8(utf8, candidate) || !editable(candidate)) return false;
    text_ = std::move(candidate); saved_ = text_;
    undo_.clear(); redo_.clear(); cursor_ = preferred_ = 0; rebuild(); return true;
}
std::string TextBuffer::utf8() const { return encodeUtf8(text_); }
void TextBuffer::rebuild() {
    starts_ = {0};
    for (std::size_t i = 0; i < text_.size(); ++i) if (text_[i] == U'\n') starts_.push_back(i + 1);
}
std::size_t TextBuffer::line() const {
    return static_cast<std::size_t>(std::upper_bound(starts_.begin(), starts_.end(), cursor_) - starts_.begin() - 1);
}
std::size_t TextBuffer::column() const { return cursor_ - starts_[line()]; }
std::u32string_view TextBuffer::lineText(std::size_t row) const {
    const auto end = row + 1 < starts_.size() ? starts_[row + 1] - 1 : text_.size();
    return std::u32string_view(text_).substr(starts_.at(row), end - starts_[row]);
}
std::size_t TextBuffer::displayColumn() const {
    std::size_t col = 0;
    for (auto c : lineText(line()).substr(0, column())) col += c == U'\t' ? 4 - col % 4 : 1;
    return col;
}
void TextBuffer::remember(std::deque<Snapshot>& history) {
    history.push_back({text_, cursor_});
    std::size_t bytes = 0;
    for (const auto& entry : history) bytes += entry.text.size() * sizeof(char32_t);
    while (history.size() > 1 && (history.size() > 100 || bytes > 16 * 1024 * 1024)) {
        bytes -= history.front().text.size() * sizeof(char32_t); history.pop_front();
    }
}
bool TextBuffer::replace(std::size_t at, std::size_t count, std::u32string_view value) {
    auto candidate = text_;
    candidate.replace(at, count, value);
    if (candidate == text_ || encodeUtf8(candidate).size() > maxBytes) return false;
    remember(undo_); redo_.clear(); text_ = std::move(candidate);
    cursor_ = at + value.size(); rebuild(); preferred_ = column(); return true;
}
bool TextBuffer::insert(std::string_view utf8) {
    std::u32string value;
    if (!decodeUtf8(utf8, value) || !editable(value)) return false;
    return replace(cursor_, 0, value);
}
bool TextBuffer::backspace() { return cursor_ && replace(cursor_ - 1, 1, {}); }
bool TextBuffer::erase() { return cursor_ < text_.size() && replace(cursor_, 1, {}); }
bool TextBuffer::undo() {
    if (undo_.empty()) return false;
    remember(redo_); auto entry = std::move(undo_.back()); undo_.pop_back();
    text_ = std::move(entry.text); cursor_ = entry.cursor; rebuild(); preferred_ = column(); return true;
}
bool TextBuffer::redo() {
    if (redo_.empty()) return false;
    remember(undo_); auto entry = std::move(redo_.back()); redo_.pop_back();
    text_ = std::move(entry.text); cursor_ = entry.cursor; rebuild(); preferred_ = column(); return true;
}
void TextBuffer::setCursor(std::size_t offset) { cursor_ = std::min(offset, text_.size()); preferred_ = column(); }
void TextBuffer::moveHorizontal(int direction) {
    if (direction < 0 && column()) --cursor_;
    else if (direction > 0 && column() < lineText(line()).size()) ++cursor_;
    preferred_ = column();
}
void TextBuffer::moveVertical(int lines) {
    const auto row = static_cast<std::size_t>(std::clamp<long long>(static_cast<long long>(line()) + lines, 0, starts_.size() - 1));
    cursor_ = starts_[row] + std::min(preferred_, lineText(row).size());
}
bool TextBuffer::goToLine(std::string_view input) {
    if (input.empty()) return false;
    std::size_t number = 0;
    const auto result = std::from_chars(input.data(), input.data() + input.size(), number);
    if (result.ec != std::errc{} || result.ptr != input.data() + input.size() || number == 0 || number > lineCount()) return false;
    setCursor(starts_[number - 1]); return true;
}
std::optional<std::size_t> TextBuffer::find(std::string_view query, std::size_t from) const {
    std::u32string needle;
    if (!decodeUtf8(query, needle) || needle.empty()) return {};
    auto found = text_.find(needle, std::min(from, text_.size()));
    if (found == std::u32string::npos) found = text_.find(needle);
    if (found == std::u32string::npos) return {};
    return found;
}
}
