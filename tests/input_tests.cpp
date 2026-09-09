#include "Input.hpp"
#include "AppState.hpp"
#include <iostream>
#include <stdexcept>
#include <array>
#include <fstream>
#include <filesystem>
#include <cstdlib>

namespace {
struct Mapping {
    SDL_Keycode key;
    SDL_GameControllerButton button;
    portpad::InputAction action;
};
constexpr std::array<Mapping, 10> mappings{{
    {SDLK_PAGEUP, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, portpad::InputAction::PageUp},
    {SDLK_PAGEDOWN, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, portpad::InputAction::PageDown},
    {SDLK_p, SDL_CONTROLLER_BUTTON_Y, portpad::InputAction::Details},
    {SDLK_UP, SDL_CONTROLLER_BUTTON_DPAD_UP, portpad::InputAction::Up},
    {SDLK_DOWN, SDL_CONTROLLER_BUTTON_DPAD_DOWN, portpad::InputAction::Down},
    {SDLK_LEFT, SDL_CONTROLLER_BUTTON_DPAD_LEFT, portpad::InputAction::Left},
    {SDLK_RIGHT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, portpad::InputAction::Right},
    {SDLK_RETURN, SDL_CONTROLLER_BUTTON_A, portpad::InputAction::Confirm},
    {SDLK_ESCAPE, SDL_CONTROLLER_BUTTON_B, portpad::InputAction::Back},
    {SDLK_TAB, SDL_CONTROLLER_BUTTON_START, portpad::InputAction::Menu},
}};
}

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(std::string(message) + ": " + SDL_GetError());
}
int main() {
    using namespace portpad;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) return 1;
    int result = 0;
    try {
        Keymap config;
        std::string error;
        check(config.load(PORTPAD_TEST_KEYMAP, error), "bundled config loads");
        for (const auto& mapping : mappings) {
            check(config.keyboard.at(mapping.key) == mapping.action, "config keyboard defaults match");
            check(config.controller.at(mapping.button) == mapping.action, "config controller defaults match");
        }
        auto pattern = (std::filesystem::temp_directory_path() / "portpad-keymap-XXXXXX").string();
        const char* directory = mkdtemp(pattern.data());
        check(directory != nullptr, "create config test directory");
        struct Cleanup {
            std::filesystem::path path;
            ~Cleanup() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
        } cleanup{directory};
        const auto customPath = cleanup.path / "keys.ini";
        std::ifstream defaults(PORTPAD_TEST_KEYMAP);
        std::string contents((std::istreambuf_iterator<char>(defaults)), {});
        contents.replace(contents.find("Confirm=Return"), 14, "Confirm=Space");
        contents.replace(contents.find("Confirm=a"), 9, "Confirm=x");
        { std::ofstream custom(customPath); custom << contents; }
        check(config.load(customPath.string(), error), "custom config loads");
        check(config.controller.at(SDL_CONTROLLER_BUTTON_X) == InputAction::Confirm, "controller remapping");
        Input remapped(customPath.string());
        SDL_Event customEvent{};
        customEvent.type = SDL_KEYDOWN;
        customEvent.key.keysym.sym = SDLK_SPACE;
        check(remapped.handle(customEvent) == InputAction::Confirm, "configured key reaches action model");
        customEvent.key.keysym.sym = SDLK_RETURN;
        check(!remapped.handle(customEvent), "old binding removed");
        { std::ofstream custom(customPath); custom << "[keyboard]\nConfirm=NotAKey\n"; }
        check(!config.load(customPath.string(), error) && !error.empty(), "invalid config rejected");
        check(config.keyboard.at(SDLK_SPACE) == InputAction::Confirm, "failed load is atomic");
        { std::ofstream custom(customPath); custom << contents << "\nConfirm=b\n"; }
        check(!config.load(customPath.string(), error), "duplicate binding rejected");
        check(!config.load((cleanup.path / "missing.ini").string(), error), "missing config rejected");
        { std::ofstream custom(customPath); custom << "[keyboard]\nConfirm=Return\n"; }
        check(!config.load(customPath.string(), error), "incomplete config rejected");
        auto collision = contents;
        collision.replace(collision.find("Menu=Tab"), 8, "Menu=Space");
        { std::ofstream custom(customPath); custom << collision; }
        check(!config.load(customPath.string(), error), "duplicate physical key rejected");
        Input fallback(customPath.string());
        check(fallback.handle(customEvent) == InputAction::Confirm, "invalid config falls back to default Return");
        std::string windows = "\xEF\xBB\xBF";
        for (const char ch : contents) { if (ch == '\n') windows += '\r'; windows += ch; }
        { std::ofstream custom(customPath, std::ios::binary); custom << windows; }
        check(config.load(customPath.string(), error), "Windows BOM and CRLF accepted");
        Input input;
        SDL_Event event{};
        event.type = SDL_QUIT;
        check(!input.handle(event), "window close is a lifecycle event");

        event.type = SDL_KEYDOWN;
        for (const auto& mapping : mappings) {
            event.key.keysym.sym = mapping.key;
            check(input.handle(event) == mapping.action, "keyboard action mapping");
            event.key.repeat = 1;
            check(!input.handle(event), "key repeat ignored");
            event.key.repeat = 0;
            event.type = SDL_KEYUP;
            check(!input.handle(event), "key release ignored");
            event.type = SDL_KEYDOWN;
        }
        event.key.keysym.sym = SDLK_BACKSPACE;
        check(!input.handle(event), "obsolete Backspace mapping is absent");
        event.key.keysym.sym = SDLK_SPACE;
        check(!input.handle(event), "unmapped key ignored");
        // Exercise a complete keyboard-only session before attaching a controller.
        AppState state;
        const auto press = [&](SDL_Keycode key) {
            event.key.keysym.sym = key;
            const auto action = input.handle(event);
            check(action.has_value(), "keyboard action exists");
            state.handle(*action);
        };
        press(SDLK_RETURN);
        press(SDLK_ESCAPE);
        check(state.running() && state.screen() == Screen::Menu, "Escape returns to menu");
        press(SDLK_TAB);
        press(SDLK_DOWN);
        press(SDLK_RETURN);
        press(SDLK_TAB);
        check(state.screen() == Screen::Menu && state.selected() == 1, "Tab returns to menu");
        press(SDLK_RIGHT);
        press(SDLK_DOWN);
        press(SDLK_RETURN);
        check(!state.running(), "keyboard can select Quit");

        const int device = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 6, 16, 1);
        check(device >= 0 && SDL_IsGameController(device), "virtual game controller attached");
        event.type = SDL_CONTROLLERDEVICEADDED;
        event.cdevice.which = device;
        input.handle(event);
        remapped.handle(event);
        check(input.controllerName() == SDL_GameControllerNameForIndex(device), "hotplug controller name");
        {
            Input startup;
            check(startup.controllerName() == input.controllerName(), "startup discovers existing controller");
        }
        event.type = SDL_CONTROLLERBUTTONDOWN;
        event.cbutton.which = SDL_JoystickGetDeviceInstanceID(device);
        const SDL_JoystickID instance = event.cbutton.which;
        event.cbutton.button = SDL_CONTROLLER_BUTTON_X;
        check(remapped.handle(event) == InputAction::Confirm, "custom controller event reaches Confirm");
        check(remapped.label(InputAction::Confirm) == "x", "controller hint follows config");
        event.cbutton.button = SDL_CONTROLLER_BUTTON_A;
        check(!remapped.handle(event), "old controller binding removed");
        for (const auto& mapping : mappings) {
            event.cbutton.button = mapping.button;
            const auto controllerAction = input.handle(event);
            check(controllerAction == mapping.action, "controller button mapping");
            SDL_Event keyboard{};
            keyboard.type = SDL_KEYDOWN;
            keyboard.key.keysym.sym = mapping.key;
            check(input.handle(keyboard) == controllerAction, "keyboard and controller actions match while connected");
        }
        event.cbutton.button = SDL_CONTROLLER_BUTTON_X;
        check(!input.handle(event), "unmapped controller button ignored");
        event.cbutton.button = SDL_CONTROLLER_BUTTON_A;
        event.cbutton.which = instance + 100;
        check(input.handle(event) == std::nullopt, "unselected controller ignored");
        event.cbutton.which = instance;
        event.type = SDL_CONTROLLERBUTTONUP;
        check(input.handle(event) == std::nullopt, "button release ignored");
        check(SDL_JoystickDetachVirtual(device) == 0, "detach virtual controller");
        event.type = SDL_CONTROLLERDEVICEREMOVED;
        event.cdevice.which = instance;
        input.handle(event);
        check(input.controllerName() == "No controller connected", "disconnect clears controller");
        event = {};
        event.type = SDL_CONTROLLERBUTTONDOWN;
        event.cbutton.which = instance;
        event.cbutton.button = SDL_CONTROLLER_BUTTON_A;
        check(!input.handle(event), "disconnected controller cannot confirm");
        event = {};
        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = SDLK_RETURN;
        check(input.handle(event) == InputAction::Confirm, "keyboard remains usable without a controller");
        const int replacement = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 6, 16, 1);
        check(replacement >= 0, "attach replacement");
        event.type = SDL_CONTROLLERDEVICEADDED;
        event.cdevice.which = replacement;
        input.handle(event);
        check(input.controllerName() == SDL_GameControllerNameForIndex(replacement), "reconnect discovers controller");
        check(SDL_JoystickDetachVirtual(replacement) == 0, "detach replacement");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        result = 1;
    }
    SDL_Quit();
    return result;
}
