#pragma once
#include "DocumentIO.hpp"
#include "TextBuffer.hpp"
#include "TextDialog.hpp"
#include "VirtualKeyboard.hpp"
#include <array>
#include <optional>
namespace portpad {
enum class EditorCommand { Backspace, Delete, Newline, Tab, Undo, Redo, Save, SaveAs, Find, GoToLine, Cancel };
class EditorState {
public:
    enum class Mode { Navigate, Keyboard, Menu, Prompt, Overwrite, CloseConfirm, EnableEditing };
    enum class Prompt { SaveAs, Find, GoToLine };
    enum class Exit { Close, Quit };
    static constexpr std::size_t fullRows = 17, inputRows = 8, columns = 55;
    inline static constexpr std::array<const char*, 9> menuItems{
        "Save", "Save As", "Undo", "Redo", "Find", "Go to line", "Close file", "Quit PortPad", "Set read-only"};
    bool open(const std::filesystem::path& path, std::string& error, bool readOnly = true);
    void handle(InputAction action);
    void command(EditorCommand command);
    void insertText(const std::string& text);
    void requestClose(Exit intent);
    std::optional<Exit> takeExit();
    bool readOnly() const { return readOnly_; }
    std::string menuLabel(std::size_t item) const { return item == 8 ? (readOnly_ ? "Start editing" : "Set read-only") : menuItems.at(item); }
    const std::filesystem::path& overwritePath() const { return target_; }
    bool loaded() const { return loaded_; }
    bool textEntry() const { return loaded_ && !notice_.active() && ((mode_ == Mode::Keyboard && !readOnly_) || mode_ == Mode::Prompt); }
    bool keyboardVisible() const { return mode_ == Mode::Keyboard || mode_ == Mode::Prompt; }
    bool menuAvailable(std::size_t item) const;
    const TextBuffer& buffer() const { return buffer_; }
    const std::filesystem::path& path() const { return document_.path; }
    Mode mode() const { return mode_; }
    std::size_t selected() const { return selected_; }
    std::size_t firstLine() const { return firstLine_; }
    std::size_t firstColumn() const { return firstColumn_; }
    const VirtualKeyboard& keyboard() const { return keyboard_; }
    const TextDialog& notice() const { return notice_; }
    const std::string& status() const { return status_; }
    const TextBuffer& promptBuffer() const { return promptBuffer_; }
    std::string promptTitle() const;
    bool crlf() const { return document_.crlf; }
    bool highlighted(std::size_t offset) const;
private:
    DocumentData document_;
    TextBuffer buffer_, promptBuffer_;
    VirtualKeyboard keyboard_;
    TextDialog notice_;
    bool loaded_ = false, readOnly_ = true;
    Mode mode_ = Mode::Navigate;
    Prompt prompt_ = Prompt::Find;
    std::size_t selected_ = 0, firstLine_ = 0, firstColumn_ = 0;
    std::optional<Exit> pendingExit_, completedExit_;
    std::optional<std::pair<std::size_t, std::size_t>> match_;
    std::string query_, status_;
    std::filesystem::path target_;
    std::optional<std::string> targetOriginal_;
    void ensureVisible();
    void beginPrompt(Prompt purpose);
    void endInput();
    void submitPrompt();
    void activateMenu();
    void findNext(bool next);
    bool saveTo(const std::filesystem::path& path, const std::optional<std::string>& original);
    void finishClose();
    void changed();
};
}
