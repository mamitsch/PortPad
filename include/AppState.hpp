#pragma once
#include "InputAction.hpp"
#include "FileBrowser.hpp"
#include "TextDialog.hpp"
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
    void requestQuit() { running_ = false; }
    std::size_t selected() const { return selected_; }
    Screen screen() const { return screen_; }
    bool running() const { return running_; }
    const FileBrowser& browser() const { return browser_; }
    const TextDialog& dialog() const { return dialog_; }
private:
    TextDialog dialog_;
    FileBrowser browser_;
    std::size_t selected_ = 0;
    Screen screen_ = Screen::Menu;
    bool running_ = true;
};
}
