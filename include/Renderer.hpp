#pragma once
#include "AppState.hpp"
#include "Input.hpp"
#include "EditorRenderer.hpp"
#include <SDL.h>
#include <SDL_ttf.h>
#include <memory>
#include <string>

namespace portpad {
class Renderer {
public:
    explicit Renderer(const std::string& fontPath);
    void draw(const AppState& state, const Input& input);
private:
    void drawDialog(const TextDialog& dialog, const Input& input);
    std::unique_ptr<EditorRenderer> editorRenderer_;
    std::string editorFontPath_;
    void text(const std::string& value, int x, int y, SDL_Color color);
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window_{nullptr, SDL_DestroyWindow};
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer_{nullptr, SDL_DestroyRenderer};
    std::unique_ptr<TTF_Font, decltype(&TTF_CloseFont)> font_{nullptr, TTF_CloseFont};
};
}
