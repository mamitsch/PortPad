#pragma once
#include "InputAction.hpp"
#include <SDL.h>
#include <map>
#include <string>

namespace portpad {
struct Keymap {
    std::map<SDL_Keycode, InputAction> keyboard;
    std::map<Uint8, InputAction> controller;
    // A complete file replaces both maps atomically; errors leave them unchanged.
    bool load(const std::string& path, std::string& error);
};
}
