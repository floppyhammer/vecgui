#include "label.h"

#include <algorithm>
#include <list>
#include <string>
#include <vector>

#include "../../resources/default_resource.h"

// See https://www.freetype.org/freetype2/docs/glyphs/glyphs-3.html for glyph conventions.

using Pathfinder::Transform2;

namespace vecgui {

std::vector<Pathfinder::Range> get_line_breakable_groups(const std::vector<LayoutGlyph> &glyphs, int offset) {
    std::vector<Pathfinder::Range> groups;

    bool rtl = false;

    if (rtl) {
        //        uint32_t group_start = glyphs.size() - 1;
        //
        //        for (int g_idx = glyphs.size() - 1; g_idx >= 0; g_idx--) {
        //            auto &g = glyphs[g_idx];
        //
        //            if (g.line_breakable_ && g_idx != glyphs.size() - 1) {
        //                Pathfinder::Range group = {group_start, group_start - g_idx};
        //                group_start = g_idx;
        //                groups.push_back(group);
        //            }
        //        }
        //
        //        Pathfinder::Range group = {group_start, group_start + 1};
        //        groups.push_back(group);
        //
        //        for (auto &r : groups) {
        //            r.begin = r.begin + 1 - r.length;
        //        }
    } else {
        uint32_t group_start = 0;

        for (int g_idx = 0; g_idx < glyphs.size(); g_idx++) {
            auto &g = glyphs[g_idx];

            if (g.line_breakable_ && g_idx != 0) {
                Pathfinder::Range group = {offset + group_start, offset + static_cast<unsigned long long>(g_idx)};
                group_start = g_idx;
                groups.push_back(group);
            }
        }

        Pathfinder::Range group = {offset + group_start, offset + (uint32_t)glyphs.size()};
        groups.push_back(group);
    }

    return groups;
}

/// PARAs -> LINEs
std::vector<Line> get_lines_with_word_wrap(float limited_width,
                                           const std::vector<Line> &original_paras,
                                           const std::vector<LayoutGlyph> &glyphs,
                                           Vec2F &out_text_size,
                                           float letter_spacing) {
    float tracking = letter_spacing;

    std::vector<Line> wrapped_lines;

    Vec2F text_size{};

    for (const auto &para : original_paras) {
        const auto &para_range = para.glyph_ranges;

        // Get glyphs in this paragraph.
        std::vector<LayoutGlyph> para_glyphs;
        for (int glyph_idx = para_range.start; glyph_idx < para_range.end; glyph_idx++) {
            para_glyphs.push_back(glyphs[glyph_idx]);
        }

        // Get line-breakable groups in this paragraph.
        auto groups_in_para_vec = get_line_breakable_groups(para_glyphs, para_range.start);
        std::list<Pathfinder::Range> groups_in_para(groups_in_para_vec.begin(), groups_in_para_vec.end());

        std::vector<float> group_widths_in_para;

        // For RTL paragraphs, we handle the groups reversely.
        if (para.rtl) {
            groups_in_para.reverse();
        }

        // Break groups that are too long.
        auto p_group = groups_in_para.begin();
        while (p_group != groups_in_para.end()) {
            auto group = *p_group;
            float group_width = 0;
            bool split_occurred = false;

            for (int j = para.rtl ? group.length() - 1 : 0; para.rtl ? j >= 0 : j < (int)group.length();
                 para.rtl ? j-- : j++) {
                int glyph_idx = group.start + j;
                const Glyph &glyph = glyphs[glyph_idx].glyph_;
                float glyph_width = glyph.x_advance;

                if (group_width == 0 && glyph_width > limited_width) {
                    // Even a single glyph doesn't fit. We must allow it to stay as its own group.
                    group_width = glyph_width + tracking;
                    break;
                }

                if ((group_width + glyph_width + tracking) > limited_width) {
                    // Split the group.
                    Pathfinder::Range range_pre;
                    Pathfinder::Range range_next;

                    if (para.rtl) {
                        range_pre = {group.start + j + 1, group.start + group.length()};
                        range_next = {group.start, group.start + j + 1};
                    } else {
                        range_pre = {group.start, group.start + (uint32_t)j};
                        range_next = {group.start + (uint32_t)j, group.start + group.length()};
                    }

                    // Update current and insert remaining.
                    *p_group = range_pre;
                    groups_in_para.insert(std::next(p_group), range_next);
                    split_occurred = true;
                    break;
                }

                group_width += glyph_width;
                group_width += tracking;
            }

            if (!split_occurred) {
                group_widths_in_para.push_back(group_width - (group_width > 0 ? tracking : 0));
                p_group++;
            }
            // else: repeat loop on p_group (which now holds range_pre) to re-split if still too long.
        }

        // Up to this point, all the groups in the paragraph meet the width requirement.
        // We can start breaking the paragraph into lines.

        int current_group_idx = 0;
        float current_line_width = 0;
        std::vector<Pathfinder::Range> current_line_groups;

        while (!groups_in_para.empty()) {
            Pathfinder::Range current_group;

            current_group = groups_in_para.front();
            groups_in_para.erase(groups_in_para.begin());

            float current_group_width = group_widths_in_para[current_group_idx];

            // Check if group is pure whitespace.
            bool is_whitespace = true;
            for (int k = current_group.start; k < current_group.end; k++) {
                if (glyphs[k].glyph_.text != " " && glyphs[k].glyph_.text != "\t" && glyphs[k].glyph_.text != "\n") {
                    is_whitespace = false;
                    break;
                }
            }

            // Handle some abonormal graphs which are too wide.
            if (current_line_groups.empty() && current_group_width > limited_width && !is_whitespace) {
                Pathfinder::Range new_range = {current_group.start, current_group.end};
                wrapped_lines.push_back({new_range, para.rtl, current_group_width});

                text_size.x = std::max(current_group_width, text_size.x);
                current_group_idx++;
                continue;
            }

            // Finish a line.
            // Whitespace groups at the end of the line don't trigger overflow (hanging whitespace).
            float effective_width = is_whitespace ? 0 : current_group_width;

            if (current_line_width + effective_width + tracking > limited_width && !current_line_groups.empty()) {
                uint32_t line_start = current_line_groups.front().start;
                uint32_t line_end = current_line_groups.front().end;

                for (auto &group : current_line_groups) {
                    line_start = std::min(line_start, (uint32_t)group.start);
                    line_end = std::max(line_end, (uint32_t)group.end);
                }

                Pathfinder::Range new_range = {line_start, line_end};
                wrapped_lines.push_back({new_range, para.rtl, current_line_width});

                text_size.x = std::max(current_line_width, text_size.x);

                current_line_groups.clear();
                current_line_width = 0;
            }

            current_line_groups.push_back(current_group);
            current_line_width += current_group_width + tracking;
            current_group_idx++;
        }

        if (!current_line_groups.empty()) {
            uint32_t line_start = current_line_groups.front().start;
            uint32_t line_end = current_line_groups.front().end;

            for (auto &group : current_line_groups) {
                line_start = std::min(line_start, (uint32_t)group.start);
                line_end = std::max(line_end, (uint32_t)group.end);
            }

            Pathfinder::Range new_range = {line_start, line_end};
            wrapped_lines.push_back({new_range, para.rtl, current_line_width});

            text_size.x = std::max(current_line_width, text_size.x);
        }
    }

    out_text_size = text_size;

    return wrapped_lines;
}

Label::Label() {
    type = NodeType::Label;
    text_ = "Label";
}

void Label::on_ready() {
    auto context = get_context();
    if (!context) {
        return;
    }

    auto default_theme = context->default_resource->get_default_theme();

    if (default_theme->font) {
        font = default_theme->font;
    } else {
        font = context->default_resource->get_default_font();
    }
}

void Label::set_text(const std::string &new_text) {
    // Only update glyphs when the text has been changed.
    if (text_ == new_text) {
        return;
    }

    text_ = new_text;
    utf8_to_utf32(text_, text_u32_);

    spans_.clear();
    spans_.push_back({.text = text_, .style = get_text_style()});

    need_to_remeasure = true;
    queue_relayout();
}

void Label::clear_spans() {
    spans_.clear();
    text_.clear();
    text_u32_.clear();
    need_to_remeasure = true;
    queue_relayout();
}

void Label::add_span(const TextSpan &span) {
    spans_.push_back(span);
    text_ += span.text;
    utf8_to_utf32(text_, text_u32_);
    need_to_remeasure = true;
    queue_relayout();
}

void Label::insert_text(uint32_t codepoint_position, const std::string &new_text) {
    if (new_text.empty()) {
        return;
    }

    assert(codepoint_position <= text_u32_.size() && "Codepoint index is out of bounds!");

    std::u32string new_text_u32;
    utf8_to_utf32(new_text, new_text_u32);

    text_u32_.insert(codepoint_position, new_text_u32);
    text_ = utf32_to_utf8(text_u32_);

    need_to_remeasure = true;
    queue_relayout();
}

void Label::remove_text(uint32_t codepoint_position, uint32_t count) {
    assert((codepoint_position + count) <= text_u32_.size() && "Codepoint index is out of bounds!");

    text_u32_.erase(codepoint_position, count);
    text_ = utf32_to_utf8(text_u32_);

    need_to_remeasure = true;
    queue_relayout();
}

std::string Label::get_sub_text(uint32_t codepoint_position, uint32_t count) const {
    assert((codepoint_position + count) <= text_u32_.size() && "Codepoint index is out of bounds!");

    auto subtext_u32 = text_u32_.substr(codepoint_position, count);
    auto subtext = utf32_to_utf8(subtext_u32);

    return subtext;
}

const std::vector<TextSpan> &Label::get_spans() const {
    return spans_;
}

std::string Label::get_text() const {
    return text_;
}

std::u32string Label::get_text_u32() const {
    return text_u32_;
}

void Label::set_size(Vec2F new_size) {
    if (size == new_size) {
        return;
    }

    size = new_size;
    queue_relayout();
}

bool is_cjk_beginning_forbidden(const std::string &text) {
    static const std::vector<std::string> forbidden = {"！", "）", "，", "。", "：", "；", "？", "》",
                                                       "」", "』", "】", "”",  "’",  "、", "·",  "!",
                                                       ")",  ",",  ".",  ":",  ";",  "?",  ">",  "]"};
    return std::find(forbidden.begin(), forbidden.end(), text) != forbidden.end();
}

bool is_cjk_ending_forbidden(const std::string &text) {
    static const std::vector<std::string> forbidden = {"（", "《", "「", "『", "【", "“", "‘", "(", "<", "[", "{"};
    return std::find(forbidden.begin(), forbidden.end(), text) != forbidden.end();
}

bool is_ideographic_script(Script script) {
    return script == Script::Han || script == Script::Hangul || script == Script::Hiragana ||
           script == Script::Katakana;
}

/// A very crude way for line-breaking.
std::vector<LayoutGlyph> convert_to_in_context_glyphs(const std::vector<Glyph> &glyphs,
                                                      const std::vector<Line> &paragraphs) {
    std::vector<LayoutGlyph> in_context_glyphs;
    in_context_glyphs.resize(glyphs.size());

    // Add line-breaking info.
    for (auto &para : paragraphs) {
        for (int glyph_idx = para.glyph_ranges.start; glyph_idx < para.glyph_ranges.end; glyph_idx++) {
            const auto &glyph = glyphs[glyph_idx];

            LayoutGlyph in_context_glyph;
            in_context_glyph.glyph_ = glyph;
            in_context_glyph.line_breakable_ = false;

            bool can_break = false;

            if (glyph_idx > para.glyph_ranges.start) {
                const auto &prev_glyph = glyphs[glyph_idx - 1];

                // 1. Break between different scripts (e.g. English to Chinese).
                if (glyph.script != prev_glyph.script) {
                    can_break = true;
                }
                // 2. Break within ideographic scripts (Chinese/Japanese).
                else if (is_ideographic_script(glyph.script)) {
                    can_break = true;
                }
                // 3. Break after a space (standard Western rule).
                else if (prev_glyph.text == " " || prev_glyph.text == "\t") {
                    can_break = true;
                }
            }

            // Kinsoku Shori (Japanese/Chinese Line Breaking Rules)
            if (can_break) {
                // Rule 1: A "beginning forbidden" character cannot start a line.
                if (is_cjk_beginning_forbidden(glyph.text)) {
                    can_break = false;
                }

                // Rule 2: A character following an "ending forbidden" character cannot start a line.
                if (glyph_idx > para.glyph_ranges.start) {
                    const auto &prev_glyph = glyphs[glyph_idx - 1];
                    if (is_cjk_ending_forbidden(prev_glyph.text)) {
                        can_break = false;
                    }
                }
            }

            in_context_glyph.line_breakable_ = can_break;
            in_context_glyphs[glyph_idx] = in_context_glyph;
        }
    }

    return in_context_glyphs;
}

void Label::measure() {
    uint32_t font_size = get_font_size();

    auto context = get_context();
    if (!context || !font) {
        return;
    }

    if (spans_.empty() && !text_.empty()) {
        // NOTE: The style (especially colors) is "baked" into the glyphs during the measurement phase.
        // If the system theme changes, labels using default colors may not update until re-measurement is triggered.
        spans_.push_back({.text = text_, .style = get_text_style()});
    }
    font->get_glyphs(context->text_server, spans_, font_size, glyphs_, paragraphs_);

    // Apply letter spacing to paragraph widths.
    for (auto &para : paragraphs_) {
        if (para.glyph_ranges.length() > 1) {
            para.width += (para.glyph_ranges.length() - 1) * letter_spacing_;
        }
    }

    // Add emoji data.
    if (emoji_font && emoji_font->is_valid()) {
        for (auto &glyph : glyphs_) {
            if (glyph.codepoints.size() == 1 && glyph.index == 0) {
                uint16_t glyph_index = emoji_font->find_glyph_index_by_codepoint(glyph.codepoints.front());
                if (glyph_index == 0) {
                    continue;
                }
                glyph.emoji = true;

                glyph.svg = emoji_font->get_glyph_svg(glyph_index);
                if (!glyph.svg.empty() && glyph.index == 0) {
                    glyph.x_advance = font_size;
                    glyph.box = {0, 0, (float)font_size, (float)font_size};
                }
            }
        }
    }

    layout_glyphs_ = convert_to_in_context_glyphs(glyphs_, paragraphs_);

    // Calculate max atomic group width using the actual line breaker logic.
    max_atomic_group_width_ = 0;
    if (word_wrap_) {
        for (const auto &para : paragraphs_) {
            const auto &para_range = para.glyph_ranges;
            std::vector<LayoutGlyph> para_glyphs;
            for (int i = para_range.start; i < para_range.end; ++i) {
                para_glyphs.push_back(layout_glyphs_[i]);
            }

            auto groups = get_line_breakable_groups(para_glyphs, para_range.start);
            for (const auto &group : groups) {
                float group_width = 0;
                for (int k = group.start; k < group.end; ++k) {
                    group_width += glyphs_[k].x_advance + letter_spacing_;
                }
                if (group_width > 0) {
                    group_width -= letter_spacing_;
                }
                max_atomic_group_width_ = std::max(max_atomic_group_width_, group_width);
            }
        }
    }
}

void Label::make_layout() {
    // Reset text's layout box.
    layout_box = RectF();

    glyph_positions.clear();

    glyph_boxes.clear();
    character_boxes.clear();

    float cursor_x = 0;
    float cursor_y = 0;

    if (word_wrap_) {
        Vec2F text_size{};
        lines_ = get_lines_with_word_wrap(size.x, paragraphs_, layout_glyphs_, text_size, letter_spacing_);
    }

    const auto &effective_line_ranges = word_wrap_ ? lines_ : paragraphs_;

    float effective_max_line_width = 0;
    if (word_wrap_) {
        effective_max_line_width = size.x;
    } else {
        for (const auto &line : effective_line_ranges) {
            effective_max_line_width = std::max(effective_max_line_width, line.width);
        }
    }

    glyph_positions.resize(glyphs_.size());

    // Build layout.
    for (const auto &line : effective_line_ranges) {
        const auto &range = line.glyph_ranges;

        switch (bidi_alignment_) {
            case BidiAlignment::Auto: {
                if (line.rtl) {
                    cursor_x = effective_max_line_width - line.width;
                }
            } break;
            case BidiAlignment::Begin: {
            } break;
            case BidiAlignment::Center: {
                cursor_x = effective_max_line_width * 0.5f - line.width * 0.5f;
            } break;
            case BidiAlignment::End: {
                cursor_x = effective_max_line_width - line.width;
            } break;
        }

        uint32_t max_font_size_in_line = get_font_size();
        float min_descent = 0;
        for (int i = range.start; i < range.end; i++) {
            const auto &g = glyphs_[i];
            max_font_size_in_line = std::max(max_font_size_in_line, g.style.font_size);
            min_descent = std::min(min_descent, g.descent);
        }

        for (int i = range.start; i < range.end; i++) {
            const auto &g = glyphs_[i];

            uint32_t current_font_size = g.style.font_size;

            // Aligns baselines by compensating for different descender depths.
            // Since descent is negative, (g.descent - min_descent) is the positive distance
            // from this glyph's baseline to the line's deepest descender.
            float descent_diff = g.descent - min_descent;

            // Calculate the height difference between the current character's box and the maximum line height.
            // Since the origin is at the top and Y is positive downward, smaller fonts need to be shifted downward.
            float bottom_offset_y = (float)(max_font_size_in_line - current_font_size - descent_diff);

            // The glyph's layout box in the text's local coordinates.
            // The origin is the top-left corner of the text box.
            RectF glyph_layout_box = RectF(cursor_x + g.x_offset,
                                           cursor_y + g.y_offset,
                                           cursor_x + g.x_advance,
                                           cursor_y + (float)current_font_size);

            // After adding bottom_offset_y, the bottom of this character will align with the line's unified bottom
            // boundary.
            glyph_positions[i] = {cursor_x + g.x_offset, cursor_y + g.y_offset + bottom_offset_y};

            // The whole text's layout box.
            layout_box = layout_box.union_rect(glyph_layout_box);

            // Advance x.
            cursor_x += g.x_advance;
            if (i < range.end - 1) {
                cursor_x += letter_spacing_;
            }
        }

        cursor_x = 0;
        cursor_y += max_font_size_in_line + line_spacing_;
    }
}

void Label::set_font(const std::shared_ptr<Font> &new_font) {
    if (new_font == nullptr) {
        return;
    }

    font = new_font;

    need_to_remeasure = true;
    queue_relayout();
}

void Label::set_font_size(uint32_t new_font_size) {
    if (get_font_size() == new_font_size) {
        return;
    }
    // Reuse old text style if there is any.
    if (!text_style_.has_value()) {
        text_style_ = get_text_style();
    }
    text_style_->font_size = new_font_size;

    need_to_remeasure = true;
    queue_relayout();
}

uint32_t Label::get_font_size() const {
    auto context = get_context();
    if (!context) {
        return 24;
    }

    auto default_theme = context->default_resource->get_default_theme();

    return text_style_.has_value() ? text_style_->font_size : default_theme->font_size;
}

void Label::set_word_wrap(bool word_wrap) {
    if (word_wrap_ == word_wrap) {
        return;
    }
    word_wrap_ = word_wrap;

    need_to_remeasure = true;
    queue_relayout();
}

void Label::consider_alignment() {
    alignment_shift = Vec2F(0);

    switch (horizontal_alignment) {
        case Alignment::Begin: {
            alignment_shift.x = -layout_box.min_x();
        } break;
        case Alignment::Center: {
            alignment_shift.x = size.x * 0.5f - layout_box.center().x;
        } break;
        case Alignment::End: {
            alignment_shift.x = size.x - layout_box.max_x();
        } break;
    }

    switch (vertical_alignment) {
        case Alignment::Begin: {
            alignment_shift.y = -layout_box.min_y();
        } break;
        case Alignment::Center: {
            alignment_shift.y = size.y * 0.5f - layout_box.center().y;
        } break;
        case Alignment::End: {
            alignment_shift.y = size.y - layout_box.max_y();
        } break;
    }
}

void Label::update(double dt) {
    NodeUi::update(dt);
}

void Label::set_text_style(TextStyle new_text_style) {
    text_style_ = new_text_style;
    if (spans_.size() <= 1) {
        spans_.clear();
        spans_.push_back({.text = text_, .style = new_text_style});
    }
    need_to_remeasure = true;
    queue_relayout();
}

void Label::draw() {
    if (!visible_) {
        return;
    }

    NodeUi::draw();

    auto global_transform = get_global_transform();

    auto context = get_context();
    if (!context) {
        return;
    }

    auto vector_server = context->vector_server;
    auto default_theme = context->default_resource->get_default_theme();

    TextStyle draw_style = get_text_style();
    auto theme_background = theme_override_bg.value_or(default_theme->label.styles["background"]);

    vector_server->draw_style_box(theme_background, global_transform, size, alpha);

    auto translation = global_transform * Transform2::from_translation(alignment_shift);

    RectF clip_box;
    //    if (clip) {
    //        clip_box = {{}, size};
    //    } else {
    //        clip_box = {{}, calc_minimum_size()};
    //    }

    vector_server->draw_glyphs(
        glyphs_, glyph_positions, draw_style, translation, clip_box, alpha, word_wrap_ ? lines_ : paragraphs_);
}

void Label::set_horizontal_alignment(Alignment alignment) {
    if (horizontal_alignment == alignment) {
        return;
    }

    horizontal_alignment = alignment;
    queue_relayout();
}

void Label::set_vertical_alignment(Alignment alignment) {
    if (vertical_alignment == alignment) {
        return;
    }

    vertical_alignment = alignment;
    queue_relayout();
}

void Label::set_bidi_alignment(BidiAlignment alignment) {
    if (bidi_alignment_ == alignment) {
        return;
    }

    bidi_alignment_ = alignment;
    queue_relayout();
}

float Label::get_line_spacing() const {
    return line_spacing_;
}

void Label::set_line_spacing(float spacing) {
    if (line_spacing_ == spacing) {
        return;
    }
    line_spacing_ = spacing;
    queue_relayout();
}

float Label::get_letter_spacing() const {
    return letter_spacing_;
}

void Label::set_letter_spacing(float spacing) {
    if (letter_spacing_ == spacing) {
        return;
    }
    letter_spacing_ = spacing;
    queue_relayout();
}

void Label::calc_minimum_size() {
    if (need_to_remeasure) {
        measure();
        need_to_remeasure = false;
    }

    auto min_size = get_text_minimum_size();

    // A Label has a minimal height even when the text is empty.
    min_size.y = std::max(min_size.y, (float)get_font_size());

    calculated_minimum_size = min_size;

    if (size.x < min_size.x) {
        size.x = min_size.x;
    }
    if (size.y < min_size.y) {
        size.y = min_size.y;
    }
}

void Label::adjust_layout() {
    make_layout();
    consider_alignment();
}

Vec2F Label::get_text_minimum_size() const {
    float effective_max_para_width = 0;

    const auto &effective_lines = word_wrap_ ? lines_ : paragraphs_;

    for (const auto &line : effective_lines) {
        effective_max_para_width = std::max(effective_max_para_width, line.width);
    }

    float total_height = 0;
    if (!effective_lines.empty()) {
        total_height = (float)effective_lines.size() * (float)get_font_size() +
                       (float)(effective_lines.size() - 1) * line_spacing_;
    }

    Vec2F text_bbox = {effective_max_para_width, total_height};

    if (word_wrap_) {
        return Vec2F(max_atomic_group_width_, text_bbox.y);
    }

    return text_bbox;
}

std::vector<Glyph> &Label::get_glyphs() {
    return glyphs_;
}

const std::vector<Vec2F> &Label::get_glyph_positions() const {
    return glyph_positions;
}

std::shared_ptr<Font> Label::get_font() const {
    return font;
}

Vec2F Label::get_alignment_shift() const {
    return alignment_shift;
}

RectF Label::get_layout_box() const {
    return layout_box;
}

float Label::get_glyph_right_edge_position(int32_t glyph_index) {
    assert(glyph_index >= 0 && "Invalid glyph index!");

    float pos = 0;

    assert(glyph_index < glyphs_.size() && "Out of bounds glyph index!");

    for (int i = 0; i <= glyph_index; i++) {
        pos += glyphs_[i].x_advance;
        if (i < (int)glyphs_.size() - 1) {
            // Check if next glyph is on the same line.
            // This is a bit simplified, ideally we'd check paragraph/line boundaries.
            pos += letter_spacing_;
        }
    }

    return pos;
}

TextStyle Label::get_text_style() const {
    // Apply theme color if not overridden.
    if (!text_style_.has_value()) {
        auto context = get_context();
        if (!context) {
            return {};
        }

        const auto default_theme = context->default_resource->get_default_theme();
        TextStyle text_style;
        text_style.color = default_theme->label.colors["text"];
        text_style.font_size = default_theme->font_size;
        return text_style;
    }

    return *text_style_;
}

float Label::get_glyph_left_edge_position(int32_t glyph_index) {
    assert(glyph_index >= 0 && "Invalid glyph index!");

    float pos = 0;

    for (int i = 0; i < glyph_index; i++) {
        pos += glyphs_[i].x_advance;
        pos += letter_spacing_;
    }

    return pos;
}

float Label::get_codepoint_right_edge_position(int32_t codepoint_index) {
    assert(codepoint_index >= 0 && "Invalid codepoint index!");

    float pos = 0;

    int32_t glyph_group_start = 0;
    int32_t glyph_group_size = 0;

    for (int i = 0; i < glyphs_.size(); i++) {
        const auto &glyph = glyphs_[i];

        if (codepoint_index >= glyph.start && codepoint_index < glyph.end) {
            glyph_group_start = i;
            glyph_group_size = codepoint_index - glyph.start + 1;
            break;
        }
    }

    for (int i = 0; i < glyphs_.size(); i++) {
        const auto &glyph = glyphs_[i];

        if (i < (glyph_group_start + glyph_group_size)) {
            pos += glyph.x_advance;
        }
    }

    return pos;
}

bool Label::get_word_wrap() const {
    return word_wrap_;
}

} // namespace vecgui
