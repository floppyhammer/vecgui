#pragma once

#include "container.h"

namespace vecgui {

class ScrollContainer : public Container {
public:
    ScrollContainer();

    void input(InputEvent &event) override;

    void update(double dt) override;

    void draw_scroll_bar();

    void pre_draw_children() override;
    void post_draw_children() override;

    void adjust_layout() override;

    void calc_minimum_size() override;

    void set_hscroll(int32_t value);

    int32_t get_hscroll() const;

    void set_vscroll(int32_t value);

    int32_t get_vscroll() const;

    bool ignore_mouse_input_outside_rect() const override {
        return true;
    }

    void set_size(Vec2F new_size) override;

    void enable_hscroll(bool enabled);
    void enable_vscroll(bool enabled);

protected:
    bool hscroll_enabled = true;
    bool vscroll_enabled = true;

    /// Scroll value can be negative to achieve blank scrolling space.
    float hscroll = 0;
    float vscroll = 0;

    Vec2F max_movement;
    Vec2F inertial_speed;
    float inertia = 100;

    float lerp_elapsed_ = 0;
    float lerp_duration_ = 2; // In second

    std::optional<float> prev_hscroll;
    std::optional<float> prev_vscroll;

    float scroll_speed = 20.0;

    StyleBox theme_scroll_bar;
    StyleBox theme_scroll_grabber;

    struct {
        Pathfinder::RenderTargetId render_target_id{};
    } temp_draw_data;
};

} // namespace vecgui
