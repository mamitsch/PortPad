#include "EditorRenderer.hpp"
#include "Utf8.hpp"
#include <algorithm>
#include <stdexcept>
namespace portpad {
namespace {
constexpr SDL_Color white{235, 240, 250, 255}, muted{165, 180, 200, 255};
constexpr SDL_Color blue{38, 86, 135, 255}, background{18, 24, 34, 255};
std::string shortened(const std::string& text, std::size_t max, bool tail = false) {
    std::u32string decoded;
    if (!decodeUtf8(text, decoded)) return "[non-UTF-8 path]";
    if (decoded.size() <= max) return text;
    return tail ? "..." + encodeUtf8(std::u32string_view(decoded).substr(decoded.size() - max + 3)) :
                  encodeUtf8(std::u32string_view(decoded).substr(0, max - 3)) + "...";
}
}
EditorRenderer::EditorRenderer(const std::string& path) {
    font_.reset(TTF_OpenFont(path.c_str(), 16));
    if (!font_ || !TTF_FontFaceIsFixedWidth(font_.get())) throw std::runtime_error("Cannot load monospaced editor font: " + path);
    int height = 0;
    if (TTF_SizeUTF8(font_.get(), "M", &cellWidth_, &height) != 0) throw std::runtime_error(TTF_GetError());
}
void EditorRenderer::panel(SDL_Renderer* renderer, int x, int y, int w, int h, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect{x, y, w, h}; SDL_RenderFillRect(renderer, &rect);
}
void EditorRenderer::text(SDL_Renderer* renderer, const std::string& value, int x, int y, SDL_Color color) {
    if (value.empty()) return;
    std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> surface(TTF_RenderUTF8_Blended(font_.get(), value.c_str(), color), SDL_FreeSurface);
    if (!surface) throw std::runtime_error(TTF_GetError());
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture(SDL_CreateTextureFromSurface(renderer, surface.get()), SDL_DestroyTexture);
    if (!texture) throw std::runtime_error(SDL_GetError());
    SDL_Rect rect{x, y, surface->w, surface->h}; SDL_RenderCopy(renderer, texture.get(), nullptr, &rect);
}
void EditorRenderer::draw(SDL_Renderer* renderer, const EditorState& editor, const Input& input) {
    panel(renderer, 0, 0, 640, 480, background);
    const auto& buffer = editor.buffer();
    text(renderer, shortened(editor.path().string(), 61, true), 10, 6, white);
    const std::string access = editor.readOnly() ? "READ ONLY" : "EDIT";
    text(renderer, access + (buffer.dirty() ? " * MODIFIED" : " / Saved"), 10, 29, buffer.dirty() ? SDL_Color{255, 205, 95, 255} : muted);
    text(renderer, editor.crlf() ? "UTF-8 / CRLF" : "UTF-8 / LF", 320, 29, muted);
    const auto count = editor.keyboardVisible() ? EditorState::inputRows : EditorState::fullRows;
    const int textX = 70, top = 56, rowHeight = 20;
    SDL_Rect clip{0, top, 634, static_cast<int>(count) * rowHeight};
    SDL_RenderSetClipRect(renderer, &clip);
    for (std::size_t row = editor.firstLine(); row < buffer.lineCount() && row < editor.firstLine() + count; ++row) {
        const int y = top + static_cast<int>(row - editor.firstLine()) * rowHeight;
        const auto number = std::to_string(row + 1);
        text(renderer, number, 62 - static_cast<int>(number.size()) * cellWidth_, y, muted);
        const auto line = buffer.lineText(row);
        std::u32string visible;
        std::size_t visual = 0;
        for (std::size_t i = 0; i < line.size(); ++i) {
            const auto width = line[i] == U'\t' ? 4 - visual % 4 : 1;
            for (std::size_t j = 0; j < width; ++j) {
                const auto col = visual + j;
                if (col >= editor.firstColumn() && col < editor.firstColumn() + EditorState::columns) {
                    if (editor.highlighted(buffer.lineStart(row) + i))
                        panel(renderer, textX + static_cast<int>(col - editor.firstColumn()) * cellWidth_, y, cellWidth_, rowHeight, blue);
                    visible += line[i] == U'\t' ? U' ' : line[i];
                }
            }
            visual += width;
            if (visual >= editor.firstColumn() + EditorState::columns) break;
        }
        text(renderer, encodeUtf8(visible), textX, y, white);
        if (row == buffer.line()) {
            const int cursorX = textX + static_cast<int>(buffer.displayColumn() - editor.firstColumn()) * cellWidth_;
            panel(renderer, cursorX, y + 1, 2, rowHeight - 2, {255, 210, 80, 255});
        }
    }
    SDL_RenderSetClipRect(renderer, nullptr);
    const auto position = "Ln " + std::to_string(buffer.line() + 1) + "/" + std::to_string(buffer.lineCount()) +
        " Col " + std::to_string(buffer.column() + 1) + "  View " + std::to_string(editor.firstLine() + 1) +
        ":" + std::to_string(editor.firstColumn() + 1);
    if (editor.keyboardVisible()) {
        panel(renderer, 0, 216, 640, 264, {24, 34, 48, 255});
        text(renderer, editor.mode() == EditorState::Mode::Prompt ? shortened(editor.promptTitle(), 62) : position, 10, 216, muted);
        text(renderer, editor.mode() == EditorState::Mode::Prompt ? shortened(editor.promptBuffer().utf8() + "|", 62, true) :
             "Text input / " + input.label(InputAction::PageUp) + ": next uppercase", 10, 239, white);
        const auto rows = editor.keyboard().rows();
        for (std::size_t row = 0; row < rows.size(); ++row) {
            const int width = 612 / static_cast<int>(rows[row].size());
            for (std::size_t col = 0; col < rows[row].size(); ++col) {
                const int x = 14 + static_cast<int>(col) * width, y = 264 + static_cast<int>(row) * 35;
                panel(renderer, x, y, width - 4, 31, row == editor.keyboard().row() && col == editor.keyboard().column() ? blue : SDL_Color{42, 52, 66, 255});
                const auto& label = rows[row][col].label;
                text(renderer, label, x + (width - 4 - static_cast<int>(label.size()) * cellWidth_) / 2, y + 5, white);
            }
        }
        text(renderer, shortened(input.label(InputAction::Confirm) + ": key  " + input.label(InputAction::Back) +
            ": bksp  " + input.label(InputAction::PageDown) + ": symbols  " + input.label(InputAction::Menu) + ": cancel/close", 63), 5, 453, muted);
    } else {
        text(renderer, position, 10, 406, muted);
        text(renderer, shortened(editor.status(), 62), 10, 428, muted);
        text(renderer, shortened(input.label(InputAction::Confirm) + (editor.readOnly() ? ": info  " : ": input  ") + input.label(InputAction::Back) + ": close  " +
            input.label(InputAction::Menu) + ": menu  " + input.label(InputAction::Details) + ": find next", 63), 5, 453, white);
    }
    if (editor.mode() == EditorState::Mode::Menu || editor.mode() == EditorState::Mode::Overwrite || editor.mode() == EditorState::Mode::CloseConfirm || editor.mode() == EditorState::Mode::EnableEditing) {
        const bool menu = editor.mode() == EditorState::Mode::Menu;
        const bool overwrite = editor.mode() == EditorState::Mode::Overwrite;
        const bool enable = editor.mode() == EditorState::Mode::EnableEditing;
        std::vector<std::string> choices;
        if (menu) for (std::size_t i = 0; i < EditorState::menuItems.size(); ++i) choices.emplace_back(editor.menuLabel(i));
        else if (overwrite) choices = {"Overwrite (make .bak)", "Cancel"};
        else if (enable) choices = {"Yes", "No"};
        else choices = {"Save", "Discard", "Cancel"};
        panel(renderer, 80, 60, 480, 381, {28, 40, 58, 255});
        text(renderer, menu ? "Editor menu" : enable ? "Switch to edit mode?" : overwrite ? "Overwrite existing file?" : "Unsaved changes", 100, 68, white);
        if (enable) text(renderer, "This will allow changes to this file.", 96, 98, white);
        if (overwrite) text(renderer, shortened(editor.overwritePath().filename().string(), 44), 96, 98, white);
        for (std::size_t i = 0; i < choices.size(); ++i) {
            const int y = (enable || overwrite ? 144 : 102) + static_cast<int>(i) * 32;
            if (i == editor.selected()) panel(renderer, 96, y, 446, 30, blue);
            text(renderer, choices[i], 108, y + 4, menu && !editor.menuAvailable(i) ? SDL_Color{102, 116, 133, 255} : white);
        }
        text(renderer, menu ? "Details: full file path / Back: cancel" : "Confirm selects / Back cancels", 96, 414, muted);
    }
}
}
