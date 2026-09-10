// Packaging-only harness: renders the application's real browser without changing it.
#include "AppState.hpp"
#include "Input.hpp"
#include "Renderer.hpp"
#include <SDL.h>
#include <SDL_ttf.h>
#include <stdexcept>
#include <iostream>

static const char* output;
static const char* font;
extern "C" void __wrap_SDL_RenderPresent(SDL_Renderer* renderer) {
    SDL_Rect banner{0, 0, 640, 22};
    SDL_SetRenderDrawColor(renderer, 120, 45, 15, 255);
    SDL_RenderFillRect(renderer, &banner);
    TTF_Font* labelFont = TTF_OpenFont(font, 14);
    if (!labelFont) throw std::runtime_error(TTF_GetError());
    SDL_Surface* label = TTF_RenderUTF8_Blended(labelFont,
        "DEVELOPMENT CAPTURE - SOFTWARE RENDERER - NOT DEVICE VALIDATED",
        SDL_Color{255, 255, 255, 255});
    if (!label) throw std::runtime_error(TTF_GetError());
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, label);
    SDL_Rect labelRect{8, 2, label->w, label->h};
    if (!texture || SDL_RenderCopy(renderer, texture, nullptr, &labelRect) != 0)
        throw std::runtime_error(SDL_GetError());
    SDL_Surface* capture = SDL_CreateRGBSurfaceWithFormat(0, 640, 480, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!capture || SDL_RenderReadPixels(renderer, nullptr, capture->format->format,
                                         capture->pixels, capture->pitch) != 0 ||
        SDL_SaveBMP(capture, output) != 0) throw std::runtime_error(SDL_GetError());
    SDL_FreeSurface(capture);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(label);
    TTF_CloseFont(labelFont);
}
int main(int argc, char** argv) {
    if (argc != 4) return 2;
    output = argv[1]; font = argv[2];
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0 || TTF_Init() != 0) return 1;
    int result = 0;
    try {
        portpad::AppState state;
        portpad::Input input(argv[3]);
        portpad::Renderer renderer(font);
        state.handle(portpad::InputAction::Confirm);
        renderer.draw(state, input);
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; result = 1; }
    TTF_Quit(); SDL_Quit();
    return result;
}
