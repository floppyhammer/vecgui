#pragma once

#include <pathfinder/prelude.h>

#include <cstdint>
#include <vector>

#include "../common/geometry.h"

namespace Pathfinder {
class Window;
}

namespace vecgui {

enum class InputEventType {
    MouseButton = 0,
    MouseMotion,
    MouseScroll,
    Key,
    Text,
    Max,
};

enum class KeyCode {
    Unknown = 0,
    LeftControl,
    LeftShift,
    C,
    V,
    X,
    R,
    Backspace,
    Left,
    Right,
    Up,
    Down,
    Delete,
    F5,
    F10,
    F11,
};

class InputEvent {
public:
    InputEventType type = InputEventType::Max;
    uint8_t window_index = 0;

    union Args {
        struct {
            KeyCode key;
            bool pressed;
            bool repeated;
        } key{};
        struct {
            uint8_t button;
            bool pressed;
            Vec2F position;
        } mouse_button;
        struct {
            float x_delta;
            float y_delta;
        } mouse_scroll;
        struct {
            Vec2F relative;
            Vec2F position;
        } mouse_motion;
        struct {
            uint32_t codepoint;
        } text;
    } args{};

    bool consumed = false;
};

/// Unicode codepoint to UTF8 string.
std::string cpp11_codepoint_to_utf8(char32_t codepoint);

/// wstring to UTF8 string.
std::string ws_to_utf8(std::wstring const &s);

/// UTF8 string to wstring.
std::wstring utf8_to_ws(std::string const &utf8);

enum class CursorShape {
    // The regular arrow cursor.
    Arrow,
    // The text input I-beam cursor shape.
    IBeam,
    // The crosshair shape.
    Crosshair,
    // The hand shape.
    Hand,
    // The horizontal resize arrow shape.
    ResizeH,
    // The vertical resize arrow shape.
    ResizeV,
    ResizeTlbr,
    ResizeTrbl,
};

struct GlfwData;

class InputServer {
public:
    static InputServer *get_singleton();

    InputServer();

    void initialize_window_callbacks(uint8_t window_index);

    void clear_events();

    std::string get_clipboard();
    void set_clipboard(const std::string &text);

    Vec2F cursor_position;
    Vec2F last_cursor_position;

    std::vector<InputEvent> input_queue;

    void set_cursor_captured(uint8_t window_index, bool captured);

    void hide_cursor(uint8_t window_index);

    void restore_cursor(uint8_t window_index);

    void set_cursor(uint8_t window_index, CursorShape shape);

    bool is_key_pressed(KeyCode code) const;

private:
    std::shared_ptr<GlfwData> glfw_data_;

    CursorShape target_cursor_shape = CursorShape::Arrow;
    CursorShape current_cursor_shape = CursorShape::Arrow;

    std::set<KeyCode> keys_pressed;
};

} // namespace vecgui
