#include "Input.hpp"
#include <iostream>

namespace portpad {
std::string Input::label(InputAction action) const {
    const SDL_Keycode keys[]{SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT, SDLK_RETURN, SDLK_ESCAPE, SDLK_TAB, SDLK_PAGEUP, SDLK_PAGEDOWN, SDLK_p};
    const SDL_GameControllerButton buttons[]{SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN,
        SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, SDL_CONTROLLER_BUTTON_A,
        SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_START, SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
        SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, SDL_CONTROLLER_BUTTON_Y};
    auto key = keys[static_cast<int>(action)];
    auto button = buttons[static_cast<int>(action)];
    if (configured_) {
        for (const auto& binding : keymap_.keyboard) if (binding.second == action) key = binding.first;
        for (const auto& binding : keymap_.controller) if (binding.second == action) button = static_cast<SDL_GameControllerButton>(binding.first);
    }
    return controller_ ? SDL_GameControllerGetStringForButton(button) : SDL_GetKeyName(key);
}
Input::Input(const std::string& configPath) {
    if (!configPath.empty()) {
        std::string error;
        configured_ = keymap_.load(configPath, error);
        if (!configured_) std::cerr << "PortPad: " << error << "; using default bindings\n";
    }
    discover();
}
Input::~Input() { if (controller_) SDL_GameControllerClose(controller_); }
void Input::discover() {
    if (controller_) return;
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            controller_ = SDL_GameControllerOpen(i);
            if (controller_) return;
            SDL_LogWarn(SDL_LOG_CATEGORY_INPUT, "Controller open failed: %s", SDL_GetError());
        }
    }
}
std::string Input::controllerName() const {
    if (!controller_) return "No controller connected";
    const char* name = SDL_GameControllerName(controller_);
    return name ? name : "Unnamed controller";
}
std::optional<InputAction> Input::handle(const SDL_Event& event) {
    if (event.type == SDL_CONTROLLERDEVICEADDED) discover();
    if (event.type == SDL_CONTROLLERDEVICEREMOVED && controller_ &&
        event.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller_))) {
        SDL_GameControllerClose(controller_);
        controller_ = nullptr;
        discover();
    }
    if (event.type == SDL_KEYDOWN && !event.key.repeat) {
        if (configured_) {
            const auto binding = keymap_.keyboard.find(event.key.keysym.sym);
            return binding == keymap_.keyboard.end() ? std::nullopt : std::optional<InputAction>(binding->second);
        }
        switch (event.key.keysym.sym) {
        case SDLK_ESCAPE: return InputAction::Back;
        case SDLK_UP: return InputAction::Up;
        case SDLK_DOWN: return InputAction::Down;
        case SDLK_LEFT: return InputAction::Left;
        case SDLK_RIGHT: return InputAction::Right;
        case SDLK_RETURN: return InputAction::Confirm;
        case SDLK_TAB: return InputAction::Menu;
        case SDLK_PAGEUP: return InputAction::PageUp;
        case SDLK_PAGEDOWN: return InputAction::PageDown;
        case SDLK_p: return InputAction::Details;
        default: break;
        }
    }
    if (event.type == SDL_CONTROLLERBUTTONDOWN && controller_ &&
        event.cbutton.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller_))) {
        if (configured_) {
            const auto binding = keymap_.controller.find(event.cbutton.button);
            return binding == keymap_.controller.end() ? std::nullopt : std::optional<InputAction>(binding->second);
        }
        switch (event.cbutton.button) {
        case SDL_CONTROLLER_BUTTON_DPAD_UP: return InputAction::Up;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return InputAction::Down;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return InputAction::Left;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return InputAction::Right;
        case SDL_CONTROLLER_BUTTON_A: return InputAction::Confirm;
        case SDL_CONTROLLER_BUTTON_B: return InputAction::Back;
        case SDL_CONTROLLER_BUTTON_START: return InputAction::Menu;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return InputAction::PageUp;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return InputAction::PageDown;
        case SDL_CONTROLLER_BUTTON_Y: return InputAction::Details;
        default: break;
        }
    }
    return std::nullopt;
}
}
