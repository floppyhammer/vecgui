#include "progress_bar.h"

#include "../../common/geometry.h"

namespace vecgui {

ProgressBar::ProgressBar() {
    type = NodeType::ProgressBar;

    theme_progress = std::optional(StyleBox());
    theme_progress.value().bg_color = ColorU(199, 84, 80);

    theme_bg = std::optional(StyleBox());

    theme_fg = std::optional(StyleBox());
    theme_fg->bg_color = ColorU();
    theme_fg->border_color = ColorU(200, 200, 200, 255);
    theme_fg->border_width = 2;

    // Don't add the label as a child since it's not a normal node but part of the button.
    label = std::make_shared<Label>();
    label->set_text("");
    label->set_mouse_filter(MouseFilter::Ignore);
    label->set_horizontal_alignment(Alignment::Center);
    label->set_vertical_alignment(Alignment::Center);
    label->set_anchor_flag(AnchorFlag::Center);

    add_embedded_child(label);
}

void ProgressBar::calc_minimum_size() {
    calculated_minimum_size = {};

    if (label_visible) {
        calculated_minimum_size = label->get_effective_minimum_size();
    }
}

void ProgressBar::update(double dt) {
    NodeUi::update(dt);

    if (lerp_enabled) {
        lerp_elapsed_ += dt;
        float t = std::clamp(lerp_elapsed_ / lerp_duration_, 0.0f, 1.0f);
        value = Pathfinder::lerp(value, target_value, t);

        ratio = (value - min_value) / (max_value - min_value);

        if (label_visible) {
            label->set_text(std::to_string((int)round(ratio * 100)) + "%");
        }
    }
}

void ProgressBar::draw() {
    if (!visible_) {
        return;
    }

    auto vector_server = VectorServer::get_singleton();

    Vec2F start_pos = get_global_position();

    switch (fill_mode_) {
        case FillMode::RightToLeft: {
            start_pos.x += size.x * (1.0f - ratio);
        } break;
        case FillMode::CenterToSide: {
            start_pos.x += size.x * (1.0f - ratio) * 0.5f;
        } break;
        case FillMode::LeftToRight:
        default: {
        } break;
    }

    if (theme_bg.has_value()) {
        vector_server->draw_style_box(theme_bg.value(), get_global_position(), size);
    }

    if (theme_progress.has_value()) {
        vector_server->draw_style_box(theme_progress.value(), start_pos, {size.x * ratio, size.y});
    }

    if (theme_fg.has_value()) {
        vector_server->draw_style_box(theme_fg.value(), get_global_position(), size);
    }
}

void ProgressBar::set_position(Vec2F new_position) {
    position = new_position;
}

void ProgressBar::set_size(Vec2F new_size) {
    size = new_size;
    label->set_size(size);
}

void ProgressBar::set_value(float new_value) {
    if (lerp_enabled) {
        target_value = std::clamp(new_value, min_value, max_value);
        lerp_elapsed_ = 0;
    } else {
        value = std::clamp(new_value, min_value, max_value);
        target_value = value; // In case lerp is enabled afterwards.
        ratio = (value - min_value) / (max_value - min_value);

        if (label_visible) {
            label->set_text(std::to_string((int)round(ratio * 100)) + "%");
        }
    }
}

float ProgressBar::get_value() const {
    return value;
}

void ProgressBar::set_min_value(float new_value) {
    min_value = new_value;
}

float ProgressBar::get_min_value() const {
    return min_value;
}

void ProgressBar::set_max_value(float new_value) {
    max_value = new_value;
}

float ProgressBar::get_max_value() const {
    return max_value;
}

void ProgressBar::set_step(float new_step) {
    step = new_step;
}

void ProgressBar::value_changed() {
    for (auto &callback : on_value_changed) {
        callback();
    }
}

void ProgressBar::connect_signal(const std::string &signal, const AnyCallable<void> &callback) {
    if (signal == "on_value_changed") {
        on_value_changed.push_back(callback);
    }
}

void ProgressBar::set_label_visibility(bool new_visibility) {
    label_visible = new_visibility;
    label->set_visibility(new_visibility);
}

void ProgressBar::set_label_font_size(float new_font_size) {
    label->set_font_size(new_font_size);
}

void ProgressBar::set_lerp_enabled(bool new_lerp_enabled) {
    lerp_enabled = new_lerp_enabled;
}

void ProgressBar::set_lerp_duration(float new_lerp_duration) {
    lerp_duration_ = new_lerp_duration;
}

void ProgressBar::set_fill_mode(FillMode new_fill_mode) {
    fill_mode_ = new_fill_mode;
}

} // namespace vecgui
