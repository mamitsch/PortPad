#include "VirtualKeyboard.hpp"
#include <algorithm>
#include <cctype>
namespace portpad {
void VirtualKeyboard::reset(bool numeric) {
    layer_ = numeric ? Layer::Digits : Layer::Letters;
    shift_ = oneShot_ = false; row_ = column_ = 0;
}
std::vector<std::vector<VirtualKey>> VirtualKeyboard::rows() const {
    std::vector<std::string> letters;
    if (layer_ == Layer::Letters) letters = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
    else if (layer_ == Layer::Digits) letters = {"1234567890", "-+.,:/=()", "_$%&@#~"};
    else letters = {"-_/\\.,:;=+", "*?!\"'()[]{", "}<>$%&|@#~"};
    std::vector<std::vector<VirtualKey>> result;
    for (const auto& row : letters) {
        std::vector<VirtualKey> keys;
        for (char c : row) {
            if (shifted()) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            std::string text(1, c); keys.push_back({text, text});
        }
        result.push_back(std::move(keys));
    }
    result.push_back({{layer_ == Layer::Digits ? "abc" : "123", "", layer_ == Layer::Digits ? KeyKind::Letters : KeyKind::Digits},
                      {shifted() ? "SHIFT" : "Shift", "", KeyKind::Shift}, {"Space", " "},
                      {"Bksp", "", KeyKind::Backspace}, {"Delete", "", KeyKind::Delete}});
    result.push_back({{layer_ == Layer::Symbols ? "abc" : "Symbols", "", layer_ == Layer::Symbols ? KeyKind::Letters : KeyKind::Symbols},
                      {"Tab", "\t"}, {"Enter", "", KeyKind::Enter}, {"Done", "", KeyKind::Done}});
    return result;
}
std::optional<VirtualKey> VirtualKeyboard::handle(InputAction action) {
    auto layout = rows();
    if (action == InputAction::Up) row_ = row_ ? row_ - 1 : layout.size() - 1;
    else if (action == InputAction::Down) row_ = (row_ + 1) % layout.size();
    else if (action == InputAction::Left) column_ = column_ ? column_ - 1 : layout[row_].size() - 1;
    else if (action == InputAction::Right) column_ = (column_ + 1) % layout[row_].size();
    else if (action == InputAction::PageUp) oneShot_ = !oneShot_;
    else if (action == InputAction::PageDown) layer_ = layer_ == Layer::Symbols ? Layer::Letters : Layer::Symbols;
    else if (action == InputAction::Confirm) {
        const auto key = layout[row_][column_];
        switch (key.kind) {
        case KeyKind::Shift: shift_ = !shift_; oneShot_ = false; break;
        case KeyKind::Letters: layer_ = Layer::Letters; break;
        case KeyKind::Digits: layer_ = Layer::Digits; break;
        case KeyKind::Symbols: layer_ = Layer::Symbols; break;
        default:
            if (key.kind == KeyKind::Text) oneShot_ = false;
            return key;
        }
    }
    layout = rows(); column_ = std::min(column_, layout[row_].size() - 1);
    return {};
}
}
