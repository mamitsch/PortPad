#pragma once
#include "EditorState.hpp"
#include "Input.hpp"
#include <SDL.h>
#include <SDL_ttf.h>
#include <memory>
namespace portpad {
class EditorRenderer {
public:
    explicit EditorRenderer(const std::string& fontPath);
    void draw(SDL_Renderer* renderer, const EditorState& editor, const Input& input);
private:
    std::unique_ptr<TTF_Font, decltype(&TTF_CloseFont)> font_{nullptr, TTF_CloseFont};
    void text(SDL_Renderer* renderer, const std::string& value, int x, int y, SDL_Color color);
    void panel(SDL_Renderer* renderer, int x, int y, int w, int h, SDL_Color color);
    int cellWidth_ = 10;
};
}
