#include "AppState.hpp"
#include "Input.hpp"
#include "Renderer.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    bool smokeTest = false;
    if (argc == 2 && std::string(argv[1]) == "--smoke-test") smokeTest = true;
    else if (argc != 1) {
        std::cerr << "Usage: portpad [--smoke-test]\n";
        return 1;
    }
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }
    if (TTF_Init() != 0) {
        std::cerr << "Font initialization failed: " << TTF_GetError() << '\n';
        SDL_Quit();
        return 1;
    }
    int result = 0;
    try {
        char* base = SDL_GetBasePath();
        if (!base) throw std::runtime_error(SDL_GetError());
        const std::string fontPath = std::string(base) + "assets/fonts/DejaVuSans.ttf";
        const char* configOverride = SDL_getenv("PORTPAD_KEYMAP");
        const std::string configPath = configOverride && *configOverride ? configOverride : std::string(base) + "config/keymap.ini";
        SDL_free(base);
        portpad::AppState state;
        portpad::Input input(configPath);
        portpad::Renderer renderer(fontPath);
        while (state.running()) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) state.requestQuit();
                else if (const auto action = input.handle(event)) state.handle(*action);
            }
            renderer.draw(state, input);
            if (smokeTest) {
                state.handle(portpad::InputAction::Confirm);
                renderer.draw(state, input);
                state.handle(portpad::InputAction::Details);
                renderer.draw(state, input);
                break;
            }
            SDL_Delay(16);
        }
    } catch (const std::exception& error) {
        std::cerr << "PortPad: " << error.what() << '\n';
        result = 1;
    }
    TTF_Quit();
    SDL_Quit();
    return result;
}
