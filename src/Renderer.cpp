#include "Renderer.hpp"
#include <stdexcept>

namespace portpad {
Renderer::Renderer(const std::string& fontPath) {
    window_.reset(SDL_CreateWindow("PortPad", SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN));
    if (!window_) throw std::runtime_error(SDL_GetError());
    renderer_.reset(SDL_CreateRenderer(window_.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
    if (!renderer_) renderer_.reset(SDL_CreateRenderer(window_.get(), -1, SDL_RENDERER_SOFTWARE));
    if (!renderer_) throw std::runtime_error(SDL_GetError());
    if (SDL_RenderSetLogicalSize(renderer_.get(), 640, 480) != 0) throw std::runtime_error(SDL_GetError());
    font_.reset(TTF_OpenFont(fontPath.c_str(), 22));
    if (!font_) throw std::runtime_error("Cannot load font " + fontPath + ": " + TTF_GetError());
}
void Renderer::text(const std::string& value, int x, int y, SDL_Color color) {
    std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> surface(
        TTF_RenderUTF8_Blended(font_.get(), value.c_str(), color), SDL_FreeSurface);
    if (!surface) throw std::runtime_error(TTF_GetError());
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture(
        SDL_CreateTextureFromSurface(renderer_.get(), surface.get()), SDL_DestroyTexture);
    if (!texture) throw std::runtime_error(SDL_GetError());
    // Clip long controller names without shrinking the text below readable size.
    SDL_Rect source{0, 0, surface->w < 640 - x - 24 ? surface->w : 640 - x - 24, surface->h};
    SDL_Rect destination{x, y, source.w, source.h};
    if (SDL_RenderCopy(renderer_.get(), texture.get(), &source, &destination) != 0)
        throw std::runtime_error(SDL_GetError());
}
void Renderer::draw(const AppState& state, const Input& input) {
    constexpr SDL_Color white{235, 240, 250, 255};
    constexpr SDL_Color muted{170, 185, 205, 255};
    SDL_SetRenderDrawColor(renderer_.get(), 18, 24, 34, 255);
    SDL_RenderClear(renderer_.get());
    text("PortPad", 24, 20, white);
    text("Controller: " + input.controllerName(), 24, 64, muted);
    if (state.screen() == Screen::Menu) {
        for (std::size_t i = 0; i < AppState::entries.size(); ++i) {
            const int y = 132 + static_cast<int>(i) * 62;
            if (i == state.selected()) {
                SDL_SetRenderDrawColor(renderer_.get(), 38, 86, 135, 255);
                SDL_Rect highlight{24, y - 5, 592, 46};
                SDL_RenderFillRect(renderer_.get(), &highlight);
            }
            text(std::string(AppState::entries[i]), 40, y, white);
        }
    } else if (state.screen() == Screen::FileBrowser) {
        const auto& browser = state.browser();
        text("Path: " + browser.path().string(), 24, 100, white);
        constexpr std::size_t visibleRows = FileBrowser::pageSize;
        const auto first = browser.selected() / visibleRows * visibleRows;
        for (std::size_t i = first; i < browser.entries().size() && i < first + visibleRows; ++i) {
            const auto& entry = browser.entries()[i];
            const int y = 138 + static_cast<int>(i - first) * 32;
            if (i == browser.selected()) {
                SDL_SetRenderDrawColor(renderer_.get(), 38, 86, 135, 255);
                SDL_Rect highlight{24, y, 592, 30};
                SDL_RenderFillRect(renderer_.get(), &highlight);
            }
            text((entry.directory ? "[DIR] " : "      ") + entry.path.filename().string(), 30, y, white);
        }
        if (browser.entries().empty()) text("No entries", 24, 138, muted);
        text(browser.message().empty() ? std::to_string(browser.entries().size()) + " entries (read-only)" :
             browser.message(), 24, 340, muted);
    } else {
        text(std::string(AppState::entries[state.selected()]), 24, 148, white);
        text(state.screen() == Screen::About ? "A gamepad-first editor for Linux handhelds." :
             "Planned for a future milestone.", 24, 210, muted);
        text(input.label(InputAction::Back) + ": back   " + input.label(InputAction::Menu) + ": menu", 24, 260, muted);
    }
    text(input.label(InputAction::Up) + "/" + input.label(InputAction::Down) + ": move   " + input.label(InputAction::Confirm) + ": confirm", 24, 382, white);
    text(state.screen() == Screen::FileBrowser ? "Select .. to open parent directory" :
         input.label(InputAction::Back) + ": back   " + input.label(InputAction::Menu) + ": menu", 24, 414, white);
    text(input.label(InputAction::PageUp) + "/" + input.label(InputAction::PageDown) + ": page   " + input.label(InputAction::Details) + ": path", 24, 446, muted);
    if (state.dialog().active()) {
        SDL_Rect panel{0, 0, 426, 480};
        SDL_SetRenderDrawColor(renderer_.get(), 28, 40, 58, 255);
        SDL_RenderFillRect(renderer_.get(), &panel);
        SDL_RenderSetClipRect(renderer_.get(), &panel);
        const auto& dialog = state.dialog();
        text(dialog.title(), 20, 16, white);
        for (std::size_t i = dialog.first(); i < dialog.lines().size() && i < dialog.first() + TextDialog::visibleRows; ++i) {
            if (!dialog.lines()[i].empty())
                text(dialog.lines()[i], 20, 58 + static_cast<int>(i - dialog.first()) * 30, white);
        }
        text("Line " + std::to_string(dialog.first() + 1) + "/" + std::to_string(dialog.lines().size()), 20, 364, muted);
        text(input.label(InputAction::Up) + "/" + input.label(InputAction::Down) + ": scroll", 20, 394, muted);
        text(input.label(InputAction::PageUp) + "/" + input.label(InputAction::PageDown) + ": page", 20, 422, muted);
        text(input.label(InputAction::Back) + ": close", 20, 450, muted);
        SDL_RenderSetClipRect(renderer_.get(), nullptr);
    }
    SDL_RenderPresent(renderer_.get());
}
}
