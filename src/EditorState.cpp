#include "EditorState.hpp"
#include "Utf8.hpp"
#include <algorithm>
namespace portpad {
bool EditorState::open(const std::filesystem::path& path, std::string& error, bool readOnly) {
    if (loaded_ && buffer_.dirty()) { error = "Close the modified document before opening another file."; return false; }
    DocumentData candidate;
    if (!DocumentIO::load(path, candidate, error)) return false;
    document_ = std::move(candidate); buffer_.load(document_.text);
    loaded_ = true; readOnly_ = readOnly; mode_ = Mode::Navigate; firstLine_ = firstColumn_ = selected_ = 0;
    match_.reset(); query_.clear(); status_.clear(); pendingExit_.reset(); completedExit_.reset(); notice_ = TextDialog{};
    return true;
}
void EditorState::ensureVisible() {
    const auto rows = keyboardVisible() ? inputRows : fullRows;
    if (buffer_.line() < firstLine_) firstLine_ = buffer_.line();
    if (buffer_.line() >= firstLine_ + rows) firstLine_ = buffer_.line() - rows + 1;
    const auto column = buffer_.displayColumn();
    if (column < firstColumn_) firstColumn_ = column;
    if (column >= firstColumn_ + columns) firstColumn_ = column - columns + 1;
}
void EditorState::changed() { match_.reset(); status_.clear(); ensureVisible(); }
bool EditorState::menuAvailable(std::size_t item) const {
    if (!loaded_) return false;
    if (item == 0) return buffer_.dirty();
    if (item == 1) return !readOnly_ || buffer_.dirty();
    if (item == 2) return !readOnly_ && buffer_.canUndo();
    if (item == 3) return !readOnly_ && buffer_.canRedo();
    return true;
}
std::string EditorState::promptTitle() const {
    if (prompt_ == Prompt::SaveAs) return "Save As (relative to current file's folder)";
    if (prompt_ == Prompt::Find) return "Find (case-sensitive); Done searches";
    return "Go to line (1-" + std::to_string(buffer_.lineCount()) + ")";
}
void EditorState::beginPrompt(Prompt purpose) {
    if (purpose == Prompt::SaveAs && readOnly_ && !buffer_.dirty()) return;
    prompt_ = purpose; mode_ = Mode::Prompt; keyboard_.reset(purpose == Prompt::GoToLine);
    promptBuffer_ = TextBuffer{};
    promptBuffer_.load(purpose == Prompt::SaveAs ? document_.path.filename().string() : purpose == Prompt::Find ? query_ : "");
    promptBuffer_.setCursor(promptBuffer_.text().size()); ensureVisible();
}
void EditorState::endInput() { mode_ = Mode::Navigate; ensureVisible(); }
void EditorState::insertText(const std::string& text) {
    if (!textEntry() || text.empty()) return;
    if (mode_ == Mode::Prompt) {
        if (text.find_first_of("\r\n\t") != std::string::npos) return;
        if (prompt_ == Prompt::GoToLine && text.find_first_not_of("0123456789") != std::string::npos) return;
        if (promptBuffer_.utf8().size() + text.size() > 4096) { notice_.open("Input limit", "Input is limited to 4096 bytes."); return; }
        promptBuffer_.insert(text);
    } else {
        if (!buffer_.insert(text)) notice_.open("Cannot insert", "Invalid text or the 1 MiB buffer limit was reached.");
        changed();
    }
}
bool EditorState::saveTo(const std::filesystem::path& path, const std::optional<std::string>& original) {
    if (readOnly_ && !buffer_.dirty()) return false;
    const auto bytes = DocumentIO::serialize(buffer_.utf8(), document_.crlf, document_.bom);
    std::string error;
    if (!DocumentIO::save(path, bytes, original, error)) { notice_.open("Save failed", error); return false; }
    document_.path = path; document_.original = bytes;
    buffer_.markSaved(); status_ = "Saved"; mode_ = Mode::Navigate;
    return true;
}
void EditorState::submitPrompt() {
    const auto value = promptBuffer_.utf8();
    if (prompt_ == Prompt::Find) {
        if (value.empty()) { notice_.open("Find", "Enter text to search for."); return; }
        query_ = value; mode_ = Mode::Navigate; findNext(false);
    } else if (prompt_ == Prompt::GoToLine) {
        if (!buffer_.goToLine(value)) { notice_.open("Invalid line", "Enter a line number from 1 to " + std::to_string(buffer_.lineCount()) + "."); return; }
        mode_ = Mode::Navigate; changed();
    } else {
        if (value.empty()) { notice_.open("Save As", "Enter a filename with a supported extension."); return; }
        target_ = std::filesystem::path(value);
        if (target_.is_relative()) target_ = document_.path.parent_path() / target_;
        target_ = target_.lexically_normal();
        if (!DocumentIO::supported(target_)) { notice_.open("Save As", "Use txt, sh, cfg, ini, gptk, json, xml or md. Backup files cannot be edited."); return; }
        std::string error;
        if (!DocumentIO::inspect(target_, targetOriginal_, error)) { notice_.open("Save As failed", error); return; }
        if (targetOriginal_) { mode_ = Mode::Overwrite; selected_ = 1; }
        else saveTo(target_, targetOriginal_);
    }
    ensureVisible();
}
void EditorState::findNext(bool next) {
    if (query_.empty()) { beginPrompt(Prompt::Find); return; }
    const auto found = buffer_.find(query_, buffer_.cursor() + (next ? 1 : 0));
    if (!found) { match_.reset(); notice_.open("Find", "No matching text found."); return; }
    std::u32string decoded; decodeUtf8(query_, decoded);
    buffer_.setCursor(*found); match_ = std::make_pair(*found, decoded.size());
    status_ = "Match; Y / Details finds next (wraps)"; ensureVisible();
}
bool EditorState::highlighted(std::size_t offset) const {
    return match_ && offset >= match_->first && offset - match_->first < match_->second;
}
void EditorState::finishClose() {
    completedExit_ = pendingExit_; pendingExit_.reset(); loaded_ = false;
    buffer_ = TextBuffer{}; document_ = DocumentData{}; mode_ = Mode::Navigate;
}
void EditorState::requestClose(Exit intent) {
    pendingExit_ = intent;
    if (!buffer_.dirty()) finishClose();
    else { mode_ = Mode::CloseConfirm; selected_ = 2; notice_ = TextDialog{}; }
}
std::optional<EditorState::Exit> EditorState::takeExit() { const auto value = completedExit_; completedExit_.reset(); return value; }
void EditorState::activateMenu() {
    if (!menuAvailable(selected_)) return;
    switch (selected_) {
    case 0: saveTo(document_.path, document_.original); break;
    case 1: beginPrompt(Prompt::SaveAs); break;
    case 2: buffer_.undo(); mode_ = Mode::Navigate; changed(); break;
    case 3: buffer_.redo(); mode_ = Mode::Navigate; changed(); break;
    case 4: beginPrompt(Prompt::Find); break;
    case 5: beginPrompt(Prompt::GoToLine); break;
    case 6: requestClose(Exit::Close); break;
    case 7: requestClose(Exit::Quit); break;
    case 8:
        if (readOnly_) { mode_ = Mode::EnableEditing; selected_ = 1; }
        else { readOnly_ = true; mode_ = Mode::Navigate; status_ = buffer_.dirty() ? "READ ONLY; unsaved edits retained" : "READ ONLY"; }
        break;
    }
}
void EditorState::command(EditorCommand action) {
    if (!loaded_ || notice_.active()) return;
    if (action == EditorCommand::Cancel) { handle(InputAction::Menu); return; }
    if (mode_ == Mode::Overwrite || mode_ == Mode::CloseConfirm || mode_ == Mode::EnableEditing) return;
    if (readOnly_ && mode_ != Mode::Prompt && (action == EditorCommand::Backspace || action == EditorCommand::Delete ||
        action == EditorCommand::Newline || action == EditorCommand::Tab || action == EditorCommand::Undo || action == EditorCommand::Redo)) return;
    auto& text = mode_ == Mode::Prompt ? promptBuffer_ : buffer_;
    switch (action) {
    case EditorCommand::Backspace: text.backspace(); break;
    case EditorCommand::Delete: text.erase(); break;
    case EditorCommand::Newline:
        if (mode_ == Mode::Prompt) { submitPrompt(); return; }
        if (!text.insert("\n")) notice_.open("Cannot insert", "The 1 MiB buffer limit was reached.");
        break;
    case EditorCommand::Tab: if (mode_ != Mode::Prompt) text.insert("\t"); break;
    case EditorCommand::Undo: text.undo(); break;
    case EditorCommand::Redo: text.redo(); break;
    case EditorCommand::Save: if (mode_ != Mode::Prompt) saveTo(document_.path, document_.original); return;
    case EditorCommand::SaveAs: if (mode_ != Mode::Prompt) beginPrompt(Prompt::SaveAs); return;
    case EditorCommand::Find: if (mode_ != Mode::Prompt) beginPrompt(Prompt::Find); return;
    case EditorCommand::GoToLine: if (mode_ != Mode::Prompt) beginPrompt(Prompt::GoToLine); return;
    case EditorCommand::Cancel: return;
    }
    if (mode_ != Mode::Prompt) changed();
}
void EditorState::handle(InputAction action) {
    if (!loaded_) return;
    if (notice_.active()) { notice_.handle(action); return; }
    if (keyboardVisible()) {
        if (mode_ == Mode::Keyboard && readOnly_) { endInput(); return; }
        if (action == InputAction::Menu) { endInput(); return; }
        if (action == InputAction::Back) {
            auto& text = mode_ == Mode::Prompt ? promptBuffer_ : buffer_;
            if (!text.backspace()) endInput();
            if (mode_ != Mode::Prompt) changed();
            return;
        }
        const auto key = keyboard_.handle(action);
        if (key) {
            if (key->kind == KeyKind::Text) insertText(key->text);
            if (key->kind == KeyKind::Backspace) command(EditorCommand::Backspace);
            if (key->kind == KeyKind::Delete) command(EditorCommand::Delete);
            if (key->kind == KeyKind::Enter) command(EditorCommand::Newline);
            if (key->kind == KeyKind::Done) { if (mode_ == Mode::Prompt) submitPrompt(); else endInput(); }
        }
    } else if (mode_ == Mode::Menu) {
        if (action == InputAction::Back || action == InputAction::Menu) mode_ = Mode::Navigate;
        else if (action == InputAction::Up) selected_ = (selected_ + menuItems.size() - 1) % menuItems.size();
        else if (action == InputAction::Down) selected_ = (selected_ + 1) % menuItems.size();
        else if (action == InputAction::Confirm) activateMenu();
        else if (action == InputAction::Details) notice_.open("Current file", document_.path.string());
    } else if (mode_ == Mode::Overwrite || mode_ == Mode::CloseConfirm || mode_ == Mode::EnableEditing) {
        const auto count = mode_ == Mode::CloseConfirm ? 3u : 2u;
        if (action == InputAction::Up || action == InputAction::Left) selected_ = (selected_ + count - 1) % count;
        else if (action == InputAction::Down || action == InputAction::Right) selected_ = (selected_ + 1) % count;
        else if (action == InputAction::Back || action == InputAction::Menu) { pendingExit_.reset(); mode_ = Mode::Navigate; }
        else if (action == InputAction::Confirm) {
            if (mode_ == Mode::EnableEditing) {
                if (selected_ == 0) { readOnly_ = false; status_ = "Editing enabled"; }
                mode_ = Mode::Navigate;
            } else if (mode_ == Mode::Overwrite) {
                if (!selected_) saveTo(target_, targetOriginal_);
                else mode_ = Mode::Prompt;
            } else if (!selected_) {
                if (saveTo(document_.path, document_.original)) finishClose();
            } else if (selected_ == 1) finishClose();
            else { pendingExit_.reset(); mode_ = Mode::Navigate; }
        }
    } else {
        switch (action) {
        case InputAction::Up: buffer_.moveVertical(-1); changed(); break;
        case InputAction::Down: buffer_.moveVertical(1); changed(); break;
        case InputAction::Left: buffer_.moveHorizontal(-1); changed(); break;
        case InputAction::Right: buffer_.moveHorizontal(1); changed(); break;
        case InputAction::PageUp: buffer_.moveVertical(-static_cast<int>(fullRows)); changed(); break;
        case InputAction::PageDown: buffer_.moveVertical(static_cast<int>(fullRows)); changed(); break;
        case InputAction::Confirm:
            if (readOnly_) notice_.open("Read only", "Use the editor menu: Start editing, then confirm Yes.");
            else { mode_ = Mode::Keyboard; keyboard_.reset(); }
            break;
        case InputAction::Back: requestClose(Exit::Close); break;
        case InputAction::Menu: mode_ = Mode::Menu; selected_ = 0; break;
        case InputAction::Details: findNext(true); break;
        }
    }
    ensureVisible();
}
}
