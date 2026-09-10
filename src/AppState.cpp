#include "AppState.hpp"

namespace portpad {
void AppState::finishEditorAction() {
    if (const auto intent = editor_.takeExit()) {
        if (*intent == EditorState::Exit::Quit) running_ = false;
        else screen_ = Screen::FileBrowser;
    }
}
void AppState::requestQuit() {
    if (editorActive()) { editor_.requestClose(EditorState::Exit::Quit); finishEditorAction(); }
    else running_ = false;
}
bool AppState::openEditor(const std::filesystem::path& path, bool readOnly) {
    std::string error;
    if (!editor_.open(path, error, readOnly)) { dialog_.open("Cannot open file", error); return false; }
    screen_ = Screen::TextEditor; return true;
}
void AppState::editorCommand(EditorCommand command) {
    if (editorActive()) { editor_.command(command); finishEditorAction(); }
}
void AppState::insertText(const std::string& text) { if (editorActive()) editor_.insertText(text); }
void AppState::handle(InputAction action) {
    if (editorActive()) { editor_.handle(action); finishEditorAction(); return; }
    if (dialog_.active()) { dialog_.handle(action); return; }
    if (fileAction_) {
        if (action == InputAction::Back || action == InputAction::Menu) fileAction_ = false;
        else if (action == InputAction::Up) fileActionSelected_ = (fileActionSelected_ + 2) % 3;
        else if (action == InputAction::Down) fileActionSelected_ = (fileActionSelected_ + 1) % 3;
        else if (action == InputAction::Confirm) {
            fileAction_ = false;
            if (fileActionSelected_ < 2) openEditor(fileActionPath_, fileActionSelected_ == 0);
        }
        return;
    }
    if (screen_ == Screen::FileBrowser && action == InputAction::Details) {
        dialog_.open("Current path", browser_.path().string());
        return;
    }
    if (screen_ == Screen::FileBrowser && action != InputAction::Menu && action != InputAction::Back) {
        if (action == InputAction::Confirm && !browser_.entries().empty()) {
            const auto& entry = browser_.entries()[browser_.selected()];
            if (!entry.directory) {
                DocumentData probe; std::string error;
                if (!DocumentIO::load(entry.path, probe, error)) dialog_.open("Cannot open file", error);
                else { fileAction_ = true; fileActionSelected_ = 0; fileActionPath_ = entry.path; }
                return;
            }
        }
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
