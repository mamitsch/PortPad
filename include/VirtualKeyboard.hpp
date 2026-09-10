#pragma once
#include "InputAction.hpp"
#include <optional>
#include <string>
#include <vector>
namespace portpad {
enum class KeyKind { Text, Backspace, Delete, Enter, Done, Shift, Letters, Digits, Symbols };
struct VirtualKey { std::string label, text; KeyKind kind = KeyKind::Text; };
class VirtualKeyboard {
public:
    enum class Layer { Letters, Digits, Symbols };
    void reset(bool numeric = false);
    std::optional<VirtualKey> handle(InputAction action);
    std::vector<std::vector<VirtualKey>> rows() const;
    std::size_t row() const { return row_; }
    std::size_t column() const { return column_; }
    bool shifted() const { return shift_ || oneShot_; }
private:
    Layer layer_ = Layer::Letters;
    bool shift_ = false, oneShot_ = false;
    std::size_t row_ = 0, column_ = 0;
};
}
