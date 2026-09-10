#pragma once
#include "InputAction.hpp"
#include "FileBrowser.hpp"
#include "TextDialog.hpp"
#include "EditorState.hpp"
#include <array>
#include <cstddef>
#include <string_view>

namespace portpad {
enum class Screen { Menu, FileBrowser, TextEditor, About };

class AppState {
public:
    inline static constexpr std::array<std::string_view, 4> entries{
        "File Browser", "Text Editor", "About", "Quit"};
    void handle(InputAction action);
    void requestQuit();
    bool openEditor(const std::filesystem::path& path, bool readOnly = true);
    bool fileActionActive() const { return fileAction_; }
    std::size_t fileActionSelected() const { return fileActionSelected_; }
    const std::filesystem::path& fileActionPath() const { return fileActionPath_; }
    void editorCommand(EditorCommand command);
    void insertText(const std::string& text);
    bool textEntry() const { return screen_ == Screen::TextEditor && editor_.textEntry(); }
    bool editorActive() const { return screen_ == Screen::TextEditor && editor_.loaded(); }
    const EditorState& editor() const { return editor_; }
    std::size_t selected() const { return selected_; }
    Screen screen() const { return screen_; }
    bool running() const { return running_; }
    const FileBrowser& browser() const { return browser_; }
    const TextDialog& dialog() const { return dialog_; }
private:
    TextDialog dialog_;
    EditorState editor_;
    bool fileAction_ = false;
    std::size_t fileActionSelected_ = 0;
    std::filesystem::path fileActionPath_;
    void finishEditorAction();
    FileBrowser browser_;
    std::size_t selected_ = 0;
    Screen screen_ = Screen::Menu;
    bool running_ = true;
};
}
