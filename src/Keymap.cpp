#include "Keymap.hpp"
#include <fstream>
#include <set>

namespace portpad {
bool Keymap::load(const std::string& path, std::string& error) {
    const std::map<std::string, InputAction> actions{
        {"Up", InputAction::Up}, {"Down", InputAction::Down}, {"Left", InputAction::Left},
        {"Right", InputAction::Right}, {"Confirm", InputAction::Confirm}, {"Back", InputAction::Back},
        {"Menu", InputAction::Menu}, {"PageUp", InputAction::PageUp},
        {"PageDown", InputAction::PageDown}, {"Details", InputAction::Details}};
    std::ifstream file(path);
    if (!file) { error = "Cannot read keymap: " + path; return false; }
    const auto trim = [](std::string value) {
        const auto first = value.find_first_not_of(" \t\r");
        return first == std::string::npos ? std::string{} : value.substr(first, value.find_last_not_of(" \t\r") - first + 1);
    };
    Keymap candidate;
    std::set<InputAction> keys, buttons;
    std::string line, section;
    int number = 0;
    while (std::getline(file, line)) {
        ++number;
        // Editors on Windows may add a UTF-8 BOM; CRLF is handled by trim.
        if (number == 1 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        if (line == "[keyboard]" || line == "[controller]") { section = line; continue; }
        const auto equal = line.find('=');
        const auto action = actions.find(trim(line.substr(0, equal)));
        bool valid = equal != std::string::npos && action != actions.end();
        if (valid) {
            const auto name = trim(line.substr(equal + 1));
            if (section == "[keyboard]") {
                const auto key = SDL_GetKeyFromName(name.c_str());
                valid = key != SDLK_UNKNOWN && !candidate.keyboard.count(key) && keys.insert(action->second).second;
                if (valid) candidate.keyboard.emplace(key, action->second);
            } else if (section == "[controller]") {
                const auto button = SDL_GameControllerGetButtonFromString(name.c_str());
                valid = button != SDL_CONTROLLER_BUTTON_INVALID && !candidate.controller.count(button) && buttons.insert(action->second).second;
                if (valid) candidate.controller.emplace(button, action->second);
            } else valid = false;
        }
        if (!valid) { error = "Invalid or duplicate keymap binding at line " + std::to_string(number); return false; }
    }
    if (file.bad() || keys.size() != actions.size() || buttons.size() != actions.size()) {
        error = "Keymap must define all ten actions once in each section"; return false;
    }
    *this = std::move(candidate);
    error.clear();
    return true;
}
}
