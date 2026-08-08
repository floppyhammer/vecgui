#pragma once

#include <functional>

#include "../../../resources/style_box.h"
#include "../container/box_container.h"
#include "../container/margin_container.h"
#include "../label.h"
#include "../node_ui.h"
#include "../texture_rect.h"

namespace vecgui {

class ToggleButtonGroup;

class Button : public NodeUi {
    friend class ToggleButtonGroup;

public:
    Button();

    void input(InputEvent &event) override;

    void draw() override;

    void set_position(Vec2F _position) override;

    void calc_minimum_size() override;

    void connect_signal(const std::string &signal, const AnyCallable<void> &callback) override;

    void set_text(const std::string &text);

    std::string get_text() const;

    void set_icon_normal(const std::shared_ptr<Image> &icon);

    void set_icon_pressed(const std::shared_ptr<Image> &icon);

    void set_flat(bool flat) {
        flat_ = flat;
    }

    /// The icon will expand until it's height matches that of the button.
    void set_icon_expand(bool enable);

    void set_toggle_mode(bool enable);

    void set_toggled(bool p_toggled);

    bool get_toggled() const {
        return toggled;
    }

    void trigger();

    /// Manual setting, no signal.
    void set_toggled_no_signal(bool new_toggled) {
        if (!toggle_mode) {
            return;
        }

        toggled = new_toggled;
    }

    void set_disabled(bool disabled) {
        disabled_ = disabled;
    }

    void set_pressed_style_to_toggled(bool value) {
        use_pressed_style_box_for_toggled = value;
    }

    ToggleButtonGroup *group = nullptr;

    // Style overrides.
    std::optional<StyleBox> theme_override_normal;
    std::optional<StyleBox> theme_override_hovered;
    std::optional<StyleBox> theme_override_pressed;
    std::optional<StyleBox> theme_override_disabled;

protected:
    bool pressed = false;
    std::optional<Vec2F> pressed_position;
    bool hovered = false;

    bool toggle_mode = false;
    bool toggled = false;
    bool use_pressed_style_box_for_toggled = false;

    bool icon_expand_ = false;

    bool flat_ = false;

    bool disabled_ = false;

    /// Embed children: Button<HBoxContainer<TextureRect, Label>>
    std::shared_ptr<MarginContainer> margin_container;
    std::shared_ptr<HBoxContainer> hbox_container;
    std::shared_ptr<TextureRect> icon_rect;
    std::shared_ptr<Label> label;

    std::shared_ptr<Image> icon_normal_;
    std::shared_ptr<Image> icon_pressed_;

    // Callbacks.
    std::vector<AnyCallable<void>> hovered_callbacks;   // Cursor entered
    std::vector<AnyCallable<void>> pressed_callbacks;   // Button is down
    std::vector<AnyCallable<void>> released_callbacks;  // Button is up
    std::vector<AnyCallable<void>> triggered_callbacks; // Button pressed then released
    std::vector<AnyCallable<void>> toggled_callbacks;   // For toggle mode only

    void notify_pressed();

    void notify_released();

    void notify_triggered();

    void notify_toggled(bool toggled);
};

class ToggleButtonGroup {
public:
    void add_button(const std::weak_ptr<Button> &new_button);

    void clear_buttons();

    void notify_toggled(const Button *toggled_button);

    std::vector<std::weak_ptr<Button>> buttons;

    bool multi_selection = false;
    uint32_t max_selection = 0;
};

} // namespace vecgui
