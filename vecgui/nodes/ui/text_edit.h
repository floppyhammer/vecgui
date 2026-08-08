#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include "../../common/geometry.h"
#include "../../resources/style_box.h"
#include "label.h"

namespace vecgui {

class MarginContainer;

/**
 * Single-Line text input field.
 */
class TextEdit : public NodeUi {
public:
    TextEdit();

    void set_text(std::string new_text) const;

    std::string get_text() const;

    void input(InputEvent &event) override;

    void update(double dt) override;

    void draw() override;

    void calc_minimum_size() override;

    void grab_focus() override;

    void release_focus() override;

    void set_editable(bool new_value);

    void set_numbers_only(bool new_value);

    std::optional<StyleBox> theme_override_normal;
    std::optional<StyleBox> theme_override_focused;
    std::optional<StyleBox> theme_override_uneditable;
    std::optional<StyleLine> theme_override_caret;
    std::optional<StyleBox> theme_override_selection;

private:
    bool editable = true;
    bool single_line = false;
    bool numbers_only = false;

    /// The codepoint which the caret currently targets to. (Codepoint, not glyph!)
    /// 0 means the caret is at the begining (before the first codepoint if there's any).
    /// codepoints.size() means the caret is at the end (after the last codepoint if there's any).
    uint32_t current_caret_index = 0;
    uint32_t selection_start_index = 0;
    Vec2F caret_position;

    std::shared_ptr<MarginContainer> margin_container;
    std::shared_ptr<Label> label;

    float caret_blink_timer = 0;

    void delete_selection();

    /// Get the closest codepoint to a mouse click.
    /// We have to pass the cursor position local to the label (the label is a child of a margin container).
    uint32_t calculate_caret_index(Vec2F local_cursor_position_to_label);

    Vec2F calculate_caret_position(int32_t caret_index);
};

} // namespace vecgui
