#pragma once
#include "InputAction.hpp"
#include "Keymap.hpp"
#include <SDL.h>
#include <optional>
#include <string>

namespace portpad {
class Input {
public:
    explicit Input(const std::string& configPath = {});
    ~Input();
    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;
    std::optional<InputAction> handle(const SDL_Event& event);
    std::string controllerName() const;
    std::string label(InputAction action) const;
private:
    Keymap keymap_;
    bool configured_ = false;
    void discover();
    SDL_GameController* controller_ = nullptr;
};
}
