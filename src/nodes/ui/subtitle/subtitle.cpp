#include "subtitle.h"

#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "nodes/ui/label.h"
#include "subtitle_style_config.h"

using json = nlohmann::json;

namespace vecgui {

// 1. 定义与 JSON 对应的字幕数据结构
struct SubtitleWord {
    std::string word;
    double start;
    double end;
    RectF rect; // 布局缓存，用于滑块和裁剪
};

struct SubtitlePhrase {
    double start{};
    double end{};
    std::string targetText;
    std::vector<SubtitleWord> targetSrt;
};

// 辅助函数：RectF 线性插值（带 Clamp 防止崩溃）
RectF lerp_rect(const RectF& a, const RectF& b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    if (!a.is_valid()) return b;
    if (!b.is_valid()) return a;
    return RectF(Pathfinder::lerp(a.left, b.left, t),
                 Pathfinder::lerp(a.top, b.top, t),
                 Pathfinder::lerp(a.right, b.right, t),
                 Pathfinder::lerp(a.bottom, b.bottom, t));
}

// 辅助函数：Transform2 线性插值
Transform2 lerp_transform(const Transform2& a, const Transform2& b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    float m11 = Pathfinder::lerp(a.m11(), b.m11(), t);
    float m21 = Pathfinder::lerp(a.m21(), b.m21(), t);
    float m12 = Pathfinder::lerp(a.m12(), b.m12(), t);
    float m22 = Pathfinder::lerp(a.m22(), b.m22(), t);
    float m13 = Pathfinder::lerp(a.m13(), b.m13(), t);
    float m23 = Pathfinder::lerp(a.m23(), b.m23(), t);
    float values[6] = {m11, m21, m12, m22, m13, m23};
    return Transform2(values);
}

void Subtitle::custom_ready() {
    setup_default_styles();

    set_anchor_flag(AnchorFlag::FullRect);

    label = std::make_shared<Label>();
    label->set_font_size(64); // 设置一个较大的字号
    label->set_horizontal_alignment(Alignment::Center);
    label->set_vertical_alignment(Alignment::Center);
    label->set_anchor_flag(AnchorFlag::FullRect);
    add_child(label);

    auto font = Font::from_file("../../assets/zcool_qingke_huangyou.ttf");
    label->set_font(font);

    debug_label_ = std::make_shared<Label>();
    debug_label_->set_anchor_flag(AnchorFlag::BottomWide);
    add_child(debug_label_);

    // 尝试从文件加载，如果文件不存在则加载默认硬编码数据
    if (!load_from_file("subtitles.json")) {
        load_default_subtitles_data();
    }
}

void Subtitle::custom_input(InputEvent& event) {
    if (event.type == InputEventType::Key && event.args.key.pressed) {
        if (event.args.key.key == KeyCode::R) { // 按 R 切换模式
            auto preset_count = subtitle_config_presets.size();
            preset_idx = (preset_idx + 1) % preset_count;
            active_subtitle_config = subtitle_config_presets[preset_idx];
            last_phrase_idx = -1; // 强制刷新
        }
    }
}

bool Subtitle::load_from_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        return false;
    }

    try {
        json data = json::parse(f);
        subtitles.clear();
        for (auto& item : data) {
            SubtitlePhrase phrase;
            phrase.start = item["start"];
            phrase.end = item["end"];
            phrase.targetText = item["targetText"];
            for (auto& s : item["targetSrt"]) {
                phrase.targetSrt.push_back({s["word"], s["start"], s["end"]});
            }
            subtitles.push_back(phrase);
        }
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

void Subtitle::custom_update(double dt) {
    // 模拟时间线进度
    current_time += dt;

    // 简单的循环播放演示
    if (!subtitles.empty() && current_time > subtitles.back().end + 1.0) {
        current_time = 0;
        last_phrase_idx = -1;
        last_word_idx = -1;
    }

    update_phrase_word_idx();

    // 统一刷新字幕显示 (基于 Span)
    refresh_subtitle_display(dt);

    debug_label_->set_text("Current phrase idx: " + std::to_string(active_phrase_idx) +
                           " | Current word idx: " + std::to_string(active_word_idx));
}

void Subtitle::draw() {
    if (!visible_) return;

    auto vs = VectorServer::get_singleton();
    auto label_origin = label->get_global_position() + label->get_alignment_shift();
    float current_alpha = modulate.a_ / 255.0f;

    // 0. 全局行背景 (在最底层绘制，跨越所有单词，支持多行)
    if (active_subtitle_config.background_color.is_visible()) {
        StyleBox sb = StyleBox::from_empty();
        sb.bg_color = active_subtitle_config.background_color;
        sb.corner_radius = active_subtitle_config.background_corner_radius;

        auto line_rects = get_line_rects();
        for (auto& r : line_rects) {
            // 外扩 padding
            auto dilated = r.dilate(active_subtitle_config.background_padding);
            vs->draw_style_box(sb, label_origin + dilated.origin(), dilated.size(), current_alpha);
        }
    }

    // 1. Slider 模式背景 (在中层绘制)
    if (active_subtitle_config.highlight_type == SubtitleHighlightType::Slider && active_phrase_idx != -1 &&
        active_word_idx != -1) {
        vs->draw_style_box(active_subtitle_config.highlight_slider.value().slider,
                           label_origin + current_slider_rect.origin(),
                           current_slider_rect.size(),
                           current_alpha);
    }

    // 2. 基础背景和边框绘制
    NodeUi::draw();

    // 注意：不要手动调用 label->draw()！
    // 因为 label 已经是子节点，引擎会自动处理它的绘制。
}

std::vector<RectF> Subtitle::get_line_rects() {
    std::vector<RectF> rects;
    auto& glyphs = label->get_glyphs();
    auto& positions = label->get_glyph_positions();
    if (glyphs.empty()) return rects;

    float current_line_y = positions[0].y;
    RectF current_line_rect;

    for (size_t i = 0; i < glyphs.size(); ++i) {
        // 换行检测
        if (std::abs(positions[i].y - current_line_y) > 1.0f) {
            if (current_line_rect.is_valid()) rects.push_back(current_line_rect);
            current_line_y = positions[i].y;
            current_line_rect = RectF();
        }

        RectF g_box;
        if (glyphs[i].skip_drawing) {
            // 对空格等不可见字符，取其行高范围以保证背景条连续
            g_box = RectF(0, -glyphs[i].ascent, glyphs[i].x_advance, glyphs[i].ascent + glyphs[i].descent);
        } else {
            g_box = glyphs[i].box;
        }

        // 核心修复：加上 glyphs[i].ascent 的偏移，以匹配渲染引擎的基线偏移逻辑
        auto render_offset = positions[i] + Vec2F(0, glyphs[i].ascent);
        current_line_rect = current_line_rect.union_rect(g_box + render_offset);
    }
    if (current_line_rect.is_valid()) rects.push_back(current_line_rect);
    return rects;
}

void Subtitle::setup_default_styles() {
    // Basic
    {
        SubtitleStyleConfig config0;
        config0.background_color = ColorU(255, 255, 0, 160);
        config0.background_padding = 8.0f;
        config0.background_corner_radius = 4.0f;
        config0.normal.color = ColorU::white();
        config0.normal.shadow_color = ColorU::black();
        config0.normal.shadow_radius = 12.0f;
        config0.normal.shadow_offset = {4, 4};
        config0.highlight_type = SubtitleHighlightType::Basic;
        auto basic_style = SubtitleHighlightBasic{};
        basic_style.highlight.color = ColorU::red();
        basic_style.highlight.bold = true;
        basic_style.highlight.italic = true;
        basic_style.highlight.shadow_color = ColorU::black();
        basic_style.highlight.shadow_radius = 12.0f;
        basic_style.highlight.shadow_offset = {4, 4};
        config0.highlight_basic = basic_style;
        config0.animation = get_subtitle_animation(SubtitleAnimationType::FadeIn);
        subtitle_config_presets.push_back(config0);

        active_subtitle_config = config0;
    }

    {
        SubtitleStyleConfig config3;
        config3.normal.color = ColorU::white();
        config3.highlight_type = SubtitleHighlightType::Basic;
        auto basic_style = SubtitleHighlightBasic{};
        basic_style.highlight.color = ColorU::blue();
        basic_style.highlight.background_color = ColorU(255, 0, 0, 160);
        basic_style.highlight.background_padding = 2.0f;
        basic_style.highlight.background_corner_radius = 4.0f;
        config3.highlight_basic = basic_style;
        config3.animation = get_subtitle_animation(SubtitleAnimationType::ScaleUp);
        subtitle_config_presets.push_back(config3);
    }

    // Slider
    {
        SubtitleStyleConfig config1;
        config1.normal.color = ColorU::white();
        config1.normal.stroke_width = 4;
        config1.normal.stroke_color = ColorU::black();
        config1.highlight_type = SubtitleHighlightType::Slider;
        auto high_style = SubtitleHighlightSlider{};
        high_style.slider.bg_color = ColorU(255, 255, 0, 80);
        high_style.slider.corner_radius = 8;
        high_style.slider.border_color = ColorU::yellow();
        high_style.slider.border_width = 1.5f;
        config1.highlight_slider = high_style;
        config1.animation = get_subtitle_animation(SubtitleAnimationType::SlideDown);
        subtitle_config_presets.push_back(config1);
    }

    // Karaoke
    {
        SubtitleStyleConfig config2;
        config2.normal.color = ColorU::white();
        config2.normal.stroke_color = ColorU::black();
        config2.normal.stroke_width = 1.5f;
        config2.highlight_type = SubtitleHighlightType::Karaoke;
        auto karaoke_style = SubtitleHighlightKaraoke{};
        karaoke_style.reached = ColorU::blue();
        karaoke_style.unreached = ColorU::white();
        config2.highlight_karaoke = karaoke_style;
        subtitle_config_presets.push_back(config2);
    }
}

void Subtitle::update_phrase_word_idx() {
    // 查找当前时间点对应的“句子”
    active_phrase_idx = -1;
    for (int i = 0; i < (int)subtitles.size(); ++i) {
        if (current_time >= subtitles[i].start && current_time <= subtitles[i].end) {
            active_phrase_idx = i;
            break;
        }
    }

    // 如果没有活动的句子，清除显示
    if (active_phrase_idx == -1) {
        if (last_phrase_idx != -1) {
            label->clear_spans();
            last_phrase_idx = -1;
            last_word_idx = -1;
        }
        return;
    }

    const auto& phrase = subtitles[active_phrase_idx];

    // 在当前句子中查找正在发音的“词”
    active_word_idx = -1;
    for (int i = 0; i < (int)phrase.targetSrt.size(); ++i) {
        if (current_time >= phrase.targetSrt[i].start && current_time <= phrase.targetSrt[i].end) {
            active_word_idx = i;
            break;
        }
    }
}

void Subtitle::refresh_subtitle_display(double dt) {
    if (active_phrase_idx == -1) {
        label->hide();
        return;
    }
    label->show();

    auto& phrase = subtitles[active_phrase_idx];
    auto sub_type = active_subtitle_config.highlight_type;
    bool has_animation = active_subtitle_config.animation.has_value();

    // 1. 判断是否需要逐帧更新 Span (Karaoke 染色或动画需要逐帧刷新)
    bool needs_per_frame_update = (sub_type == SubtitleHighlightType::Karaoke) || has_animation;

    // 2. 如果不需要逐帧刷新，只有在词/句切换时才更新
    bool phrase_changed = active_phrase_idx != last_phrase_idx;
    bool word_changed = active_word_idx != last_word_idx;

    if (needs_per_frame_update || phrase_changed || word_changed) {
        // 如果句子发生了变化，先进行一次“干跑”布局计算，以获取准确的单词矩形
        if (phrase_changed) {
            label->clear_spans();
            for (auto& w : phrase.targetSrt) {
                label->add_span({w.word, active_subtitle_config.normal});
            }
            label->calc_minimum_size();
            label->adjust_layout();
            calculate_word_rects(phrase);
        }

        label->clear_spans();

        for (int i = 0; i < (int)phrase.targetSrt.size(); ++i) {
            auto& word_data = phrase.targetSrt[i];
            TextStyle style = active_subtitle_config.normal;

            // 计算当前词的进度 (0.0 - 1.0)
            double progress = 0;
            if (current_time >= word_data.start && current_time <= word_data.end) {
                progress = (current_time - word_data.start) / (word_data.end - word_data.start);
            }

            // A. 高亮逻辑
            if (sub_type == SubtitleHighlightType::Basic) {
                if (i == active_word_idx) {
                    style = active_subtitle_config.highlight_basic->highlight;
                }
            } else if (sub_type == SubtitleHighlightType::Karaoke) {
                auto& karaoke = *active_subtitle_config.highlight_karaoke;
                if (current_time > word_data.end) {
                    style.color = karaoke.reached;
                    style.karaoke_progress = -1.0f;
                } else if (current_time >= word_data.start) {
                    style.color = karaoke.unreached;
                    style.karaoke_reached_color = karaoke.reached;
                    style.karaoke_progress = (float)progress;
                } else {
                    style.color = karaoke.unreached;
                    style.karaoke_progress = -1.0f;
                }
            }

            // B. 动画逻辑：应用以单词中心为原点的变换
            if (has_animation) {
                auto& anim = *active_subtitle_config.animation;
                Transform2 anim_xform;
                float current_alpha = style.alpha;

                if (current_time > word_data.end) {
                    anim_xform = anim.transform_end;
                    current_alpha = anim.alpha_end;
                } else if (current_time >= word_data.start) {
                    anim_xform = lerp_transform(anim.transform_start, anim.transform_end, (float)progress);
                    current_alpha = Pathfinder::lerp(anim.alpha_start, anim.alpha_end, (float)progress);
                } else {
                    anim_xform = anim.transform_start;
                    current_alpha = anim.alpha_start;
                }

                // 核心修复：如果矩形有效，则绕中心缩放/旋转
                if (word_data.rect.is_valid()) {
                    Vec2F center = word_data.rect.center();
                    style.local_transform =
                        Transform2::from_translation(center) * anim_xform * Transform2::from_translation(-center);
                } else {
                    style.local_transform = anim_xform;
                }
                style.alpha = current_alpha;
            }

            label->add_span({word_data.word, style});
        }

        label->calc_minimum_size();
        label->adjust_layout();

        last_phrase_idx = active_phrase_idx;
        last_word_idx = active_word_idx;
    }

    // 3. Slider 模式背景移动逻辑 (独立于 Span 的样式，但依赖于单词位置)
    if (sub_type == SubtitleHighlightType::Slider) {
        update_slider_logic(dt);
    }
}

void Subtitle::update_slider_logic(double dt) {
    auto& phrase = subtitles[active_phrase_idx];
    if (active_word_idx != -1) {
        target_slider_rect = phrase.targetSrt[active_word_idx].rect.dilate(4);
    }
    // 第一词直接到位
    if (active_word_idx == 0) {
        current_slider_rect = target_slider_rect;
    } else {
        current_slider_rect = lerp_rect(current_slider_rect, target_slider_rect, 10.0f * (float)dt);
    }
}

void Subtitle::calculate_word_rects(SubtitlePhrase& phrase) {
    auto& glyphs = label->get_glyphs();
    auto& positions = label->get_glyph_positions();

    std::u32string phrase_str;
    utf8_to_utf32(phrase.targetText, phrase_str);

    int g_idx = 0;
    for (auto& w : phrase.targetSrt) {
        std::u32string w32;
        utf8_to_utf32(w.word, w32);
        RectF w_rect;
        int count = 0;
        while (count < (int)w32.size() && g_idx < (int)glyphs.size()) {
            // 修正坐标偏移：将 Y 从基线原点修正到行顶端
            auto offset = positions[g_idx] + Vec2F(0, glyphs[g_idx].ascent);
            w_rect = w_rect.union_rect(glyphs[g_idx].box + offset);
            g_idx++;
            count++;
        }
        w.rect = w_rect;
    }
}

void Subtitle::load_default_subtitles_data() {
    subtitles = {
        {0.034, 1.059, "大家好", {{"大家", 0.034, 0.717}, {"好", 0.717, 1.059}}},
        {1.1, 1.899, "认识 VecGui", {{"认识", 1.1, 1.26}, {" ", 1.26, 1.34}, {"VecGui", 1.34, 1.899}}},
        {2.399, 4.219, "立即生成字幕", {{"立即", 2.399, 3.006}, {"生成", 3.006, 3.612}, {"字幕", 3.612, 4.219}}},
        {4.259,
         6.44,
         "并将它们翻译成多种语言",
         {{"并", 4.259, 4.457},
          {"将", 4.457, 4.656},
          {"它们", 4.656, 5.052},
          {"翻译成", 5.052, 5.647},
          {"多种语言", 5.647, 6.44}}},
        {6.48, 7.499, "只需一键", {{"只", 6.48, 6.735}, {"需", 6.735, 6.989}, {"一键", 6.989, 7.499}}}};
}

} // namespace vecgui