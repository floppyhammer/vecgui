#pragma once

#include <optional>

#include "../../../resources/style_box.h"
#include "../button/button.h"
#include "../label.h"
#include "box_container.h"

namespace vecgui {

class SplitContainer : public Container {
public:
    SplitContainer();

    void update(double dt) override;

    void draw() override;

    void calc_minimum_size() override;

    void adjust_layout() override;

    void input(InputEvent &event) override;

    void connect_signal(const std::string &signal, const AnyCallable<void> &callback) override;

    void set_separation(float new_separation);

    void set_split_ratio(float new_ratio);

private:
    void when_grabber_moved();

    std::vector<AnyCallable<void>> grabber_moved_callbacks;

    Vec2F size_before_collapse_;

    bool cursor_in_ = false;

    bool splitting_enabled_ = true;

    float split_to_right_length = 200;
    std::optional<Vec2F> grabber_pressed_pos_;
    Vec2F grabber_pressed_offset;
    float grabber_size_ = 8;

    std::optional<StyleBox> theme_title_bar_;
    std::optional<StyleBox> theme_panel_;
};

} // namespace vecgui
