#pragma once

#include <cstdint>
#include <memory>

#include "../../common/geometry.h"
#include "../../resources/font.h"
#include "../../resources/style_box.h"
#include "node_ui.h"

using Pathfinder::Rect;

namespace vecgui {

enum class BidiAlignment {
    Auto,
    Begin,
    Center,
    End,
};

enum class Alignment {
    Begin,
    Center,
    End,
};

struct LayoutGlyph {
    Glyph glyph_;
    bool line_breakable_ = false;
};

class Label : public NodeUi {
public:
    Label();

    void set_text(const std::string &new_text);

    void clear_spans();
    void add_span(const TextSpan &span);
    const std::vector<TextSpan>& get_spans() const { return spans_; }

    std::string get_text() const;

    std::u32string get_text_u32() const;

    void insert_text(uint32_t codepoint_position, const std::string &new_text);

    void remove_text(uint32_t codepoint_position, uint32_t count);

    std::string get_sub_text(uint32_t codepoint_position, uint32_t count) const;

    void set_size(Vec2F new_size) override;

    void set_font(std::shared_ptr<Font> new_font);

    void set_font_size(uint32_t new_font_size);

    uint32_t get_font_size() const {
        return font_size_;
    }

    void set_text_style(TextStyle new_text_style);

    void update(double dt) override;

    void draw() override;

    void set_horizontal_alignment(Alignment alignment);

    void set_vertical_alignment(Alignment alignment);

    void calc_minimum_size() override;

    void adjust_layout() override;

    std::vector<Glyph> &get_glyphs();

    std::shared_ptr<Font> get_font() const;

    float get_glyph_left_edge_position(int32_t glyph_index);

    float get_glyph_right_edge_position(int32_t glyph_index);

    /// Get the caret position of a given codepoint index.
    float get_codepoint_right_edge_position(int32_t codepoint_index);

    bool get_word_wrap() const {
        return word_wrap_;
    }

    void set_word_wrap(bool word_wrap);

    void set_multi_line(bool enabled);

    std::optional<StyleBox> theme_override_bg;
    std::optional<TextStyle> theme_override_text_style;
    TextStyle text_style;

private:
    void measure();

    void make_layout();

    void consider_alignment();

    /// The minimum size of the text box, which is determined by the text content.
    Vec2F get_text_minimum_size() const;

private:
    // Rich text segments.
    std::vector<TextSpan> spans_;

    // Raw text.
    std::string text_;
    // Codepoint-separated text.
    std::u32string text_u32_;

    /// text_u32_ is related to navigation, glyphs_ is more about rendering.

    std::shared_ptr<Font> font, emoji_font;

    uint32_t font_size_;

    bool clip = false;

    bool multi_line_ = false;

    /// If automatically break lines at suitable positions.
    bool word_wrap_ = false;

    // Layout-independent. Glyph count will not necessarily be the same as the character count.
    std::vector<Glyph> glyphs_;

    std::vector<LayoutGlyph> layout_glyphs_;

    // Layout-dependent. Ranges for glyphs, not for characters.
    std::vector<Line> paragraphs_;

    // If word_wrap is enabled, use this instead of para_ranges.
    std::vector<Line> lines_;

    // Layout-dependent.
    std::vector<Vec2F> glyph_positions;

    mutable RectF layout_box;

    std::vector<RectF> glyph_boxes;
    std::vector<RectF> character_boxes;

    bool need_to_remeasure = true;
    bool need_to_update_layout = true;

    // Controls how to align the text box to the label area.
    Alignment horizontal_alignment = Alignment::Center;
    Alignment vertical_alignment = Alignment::Center;
    Vec2F alignment_shift{0};

    BidiAlignment bidi_alignment_ = BidiAlignment::Auto;
};

} // namespace vecgui
