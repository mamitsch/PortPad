#include "AppState.hpp"
#include <iostream>
#include <stdexcept>

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
int main() {
    using namespace portpad;
    try {
        AppState state;
        check(state.running() && state.selected() == 0 && state.screen() == Screen::Menu, "initial menu");
        state.handle(InputAction::Back);
        check(state.running() && state.screen() == Screen::Menu, "back at root stays open");
        state.handle(InputAction::Up);
        check(state.selected() == 3, "up wraps");
        state.handle(InputAction::Down);
        check(state.selected() == 0, "down wraps");
        state.handle(InputAction::Left);
        check(state.selected() == 3, "left wraps");
        state.handle(InputAction::Right);
        check(state.selected() == 0, "right wraps");
        state.handle(InputAction::Confirm);
        state.handle(InputAction::Details);
        check(state.dialog().active(), "path dialog opens");
        const auto path = state.browser().path();
        state.handle(InputAction::Back);
        check(!state.dialog().active() && state.browser().path() == path, "modal back does not navigate browser");
        TextDialog dialog;
        const std::string longPath(800, 'x');
        dialog.open("Path", longPath);
        std::string joined;
        for (const auto& line : dialog.lines()) joined += line;
        check(joined == longPath, "dialog preserves full text");
        dialog.handle(InputAction::PageDown);
        check(dialog.first() == TextDialog::visibleRows, "dialog page down");
        for (int i = 0; i < 100; ++i) dialog.handle(InputAction::Down);
        check(dialog.first() + TextDialog::visibleRows == dialog.lines().size(), "dialog bottom clamp");
        for (int i = 0; i < 100; ++i) dialog.handle(InputAction::PageUp);
        check(dialog.first() == 0, "dialog top clamp");
        dialog.open("UTF-8", std::string(15, 'x') + "éé");
        check(dialog.lines()[0] == std::string(15, 'x') + "é" && dialog.lines()[1] == "é", "UTF-8 wrapping");
        state.handle(InputAction::Menu);
        check(state.screen() == Screen::Menu && state.selected() == 0, "menu returns from detail");
        state.handle(InputAction::Menu);
        check(state.running() && state.screen() == Screen::Menu, "menu at root stays open");
        for (const auto screen : {Screen::FileBrowser, Screen::TextEditor, Screen::About}) {
            const auto selected = state.selected();
            state.handle(InputAction::Confirm);
            check(state.screen() == screen, "selection opens expected screen");
            state.handle(InputAction::Down);
            state.handle(InputAction::Confirm);
            check(state.selected() == selected && state.screen() == screen, "menu input ignored on detail screen");
            state.handle(screen == Screen::FileBrowser ? InputAction::Menu : InputAction::Back);
            check(state.screen() == Screen::Menu && state.selected() == selected, "back preserves selection");
            state.handle(InputAction::Down);
        }
        state.handle(InputAction::Confirm);
        check(!state.running(), "quit entry exits");
        AppState other;
        other.handle(InputAction::Confirm);
        other.requestQuit();
        check(!other.running(), "quit works on detail screen");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
