#pragma once

#include <functional>
#include <optional>

#include "../../resources/style_box.h"
#include "label.h"
#include "node_ui.h"

namespace vecgui {

class ProgressBar : public NodeUi {
public:
    ProgressBar();

    enum class FillMode {
        LeftToRight,
        RightToLeft,
        CenterToSide,
    };

    void update(double dt) override;

    void draw() override;

    void set_position(Vec2F new_position) override;

    void set_size(Vec2F new_size) override;

    void calc_minimum_size() override;

    void set_value(float new_value);

    float get_value() const;

    void set_min_value(float new_value);

    float get_min_value() const;

    void set_max_value(float new_value);

    float get_max_value() const;

    void set_step(float new_step);

    void value_changed();

    void connect_signal(const std::string &signal, const AnyCallable<void> &callback) override;

    void set_label_visibility(bool new_visibility);

    void set_label_font_size(float new_font_size);

    void set_lerp_enabled(bool new_lerp_enabled);

    void set_lerp_duration(float new_lerp_duration);

    void set_fill_mode(FillMode new_fill_mode);

protected:
    float value = 50;
    float target_value = 50;
    float min_value = 0;
    float max_value = 100;
    float step = 1;
    float ratio = 0;

    bool label_visible = true;
    FillMode fill_mode_ = FillMode::LeftToRight;

    bool lerp_enabled = false;
    float lerp_elapsed_ = 0;
    float lerp_duration_ = 0.5; // In second

    std::optional<StyleBox> theme_progress, theme_bg, theme_fg;

    std::shared_ptr<Label> label;

    std::vector<AnyCallable<void>> on_value_changed;
};

} // namespace vecgui
