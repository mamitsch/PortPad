#include "AppState.hpp"

namespace portpad {
void AppState::handle(InputAction action) {
    if (dialog_.active()) { dialog_.handle(action); return; }
    if (screen_ == Screen::FileBrowser && action == InputAction::Details) {
        dialog_.open("Current path", browser_.path().string());
        return;
    }
    if (screen_ == Screen::FileBrowser && action != InputAction::Menu && action != InputAction::Back) {
        browser_.handle(action);
        return;
    }
    if (action == InputAction::Back || action == InputAction::Menu) {
        screen_ = Screen::Menu;
        return;
    }
    if (screen_ != Screen::Menu) return;
    switch (action) {
    case InputAction::Left:
    case InputAction::Up: selected_ = (selected_ + entries.size() - 1) % entries.size(); break;
    case InputAction::Right:
    case InputAction::Down: selected_ = (selected_ + 1) % entries.size(); break;
    case InputAction::Confirm:
        switch (selected_) {
        case 0:
            browser_.open(browser_.path().empty() ? std::filesystem::path(".") : browser_.path());
            screen_ = Screen::FileBrowser;
            break;
        case 1: screen_ = Screen::TextEditor; break;
        case 2: screen_ = Screen::About; break;
        case 3: running_ = false; break;
        }
        break;
    default: break;
    }
}
}
