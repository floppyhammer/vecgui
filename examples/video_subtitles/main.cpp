#include <resources/default_resource.h>

#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "app.h"

using json = nlohmann::json;

using namespace vecgui;

// 1. 字幕样式类型
enum class SubtitleType {
    Basic,
    Slider,  // 滑块背景随文字滑动
    Karaoke, // 逐字进度染色效果
};

struct SubtitleBasic {
    TextStyle normal;
    TextStyle highlight;
    Vec2F highlight_transform;
};

struct SubtitleSlider {
    TextStyle normal;
    TextStyle highlight;
    StyleBox highlight_slider;
};

struct SubtitleKaraoke {
    TextStyle normal;
    ColorU reached;
    ColorU unreached;
};

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

// 2. 自定义字幕显示节点
class SubtitleNode : public NodeUi {
public:
    SubtitleType sub_type = SubtitleType::Basic; // 默认 Slider 模式

    SubtitleBasic basic_style;
    SubtitleSlider slider_style;
    SubtitleKaraoke karaoke_style;

    void custom_ready() override {
        setup_default_styles();

        set_anchor_flag(AnchorFlag::FullRect);

        label = std::make_shared<Label>();
        label->set_font_size(64); // 设置一个较大的字号
        label->set_horizontal_alignment(Alignment::Center);
        label->set_vertical_alignment(Alignment::Center);
        label->set_anchor_flag(AnchorFlag::FullRect);
        add_child(label);

        debug_label_ = std::make_shared<Label>();
        debug_label_->set_anchor_flag(AnchorFlag::BottomWide);
        add_child(debug_label_);

        // 尝试从文件加载，如果文件不存在则加载默认硬编码数据
        if (!load_from_file("subtitles.json")) {
            load_default_subtitles_data();
        }
    }

    void custom_input(InputEvent& event) override {
        if (event.type == InputEventType::Key && event.args.key.pressed) {
            if (event.args.key.key == KeyCode::R) { // 按 R 切换模式
                sub_type = static_cast<SubtitleType>((static_cast<int>(sub_type) + 1) % 3);
                last_phrase_idx = -1; // 强制刷新
                printf("Switched Subtitle Mode to: %d\n", (int)sub_type);
            }
        }
    }

    bool load_from_file(const std::string& path) {
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

    void custom_update(double dt) override {
        // 模拟时间线进度
        current_time += dt;

        // 简单的循环播放演示
        if (!subtitles.empty() && current_time > subtitles.back().end + 1.0) {
            current_time = 0;
            last_phrase_idx = -1;
            last_word_idx = -1;
        }

        update_phrase_word_idx();

        if (sub_type == SubtitleType::Basic) {
            update_logic_for_basic();
        } else if (sub_type == SubtitleType::Slider) {
            update_logic_for_slider(dt);
        } else if (sub_type == SubtitleType::Karaoke) {
            update_logic_for_karaoke(dt);
        }

        debug_label_->set_text("Current phrase idx: " + std::to_string(active_phrase_idx) +
                               " | Current word idx: " + std::to_string(active_word_idx));
    }

    void draw() override {
        if (!visible_) return;

        auto vs = VectorServer::get_singleton();
        auto label_origin = label->get_global_position() + label->get_alignment_shift();
        float current_alpha = modulate.a_ / 255.0f;

        // 1. Slider 模式背景 (在文字底层绘制)
        if (sub_type == SubtitleType::Slider && active_phrase_idx != -1 && active_word_idx != -1) {
            vs->draw_style_box(slider_style.highlight_slider,
                               label_origin + current_slider_rect.origin(),
                               current_slider_rect.size(),
                               current_alpha);
        }

        // 2. 基础背景和边框绘制
        NodeUi::draw();

        // 注意：不要手动调用 label->draw()！
        // 因为 label 已经是子节点，引擎会自动处理它的绘制。
    }

private:
    void setup_default_styles() {
        // Basic
        basic_style.normal.color = ColorU::white();
        basic_style.normal.shadow_color = ColorU::black();
        basic_style.normal.shadow_radius = 12.0f;
        basic_style.highlight.color = ColorU::red();
        basic_style.highlight.bold = true;
        basic_style.highlight.shadow_color = ColorU::black();
        basic_style.highlight.shadow_radius = 12.0f;

        // Slider
        slider_style.highlight_slider.bg_color = ColorU(255, 255, 0, 80);
        slider_style.highlight_slider.corner_radius = 8;
        slider_style.highlight_slider.border_color = ColorU::yellow();
        slider_style.highlight_slider.border_width = 1.5f;

        // Karaoke
        karaoke_style.reached = ColorU::blue();
        karaoke_style.unreached = ColorU::white();
        karaoke_style.normal.stroke_color = ColorU::black();
        karaoke_style.normal.stroke_width = 1.5f;
    }

    void update_phrase_word_idx() {
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

    void update_logic_for_basic() {
        if (active_phrase_idx == -1 || active_word_idx == -1) {
            return;
        }

        const auto& phrase = subtitles[active_phrase_idx];

        // 性能优化：只有在句子切换或高亮词切换时才重新构建富文本 Span
        if (active_phrase_idx != last_phrase_idx || active_word_idx != last_word_idx) {
            label->clear_spans();

            // 基础样式
            TextStyle normal_style = basic_style.normal;

            // 高亮样式 (当前正在说的词)
            TextStyle highlight_style = basic_style.highlight;

            // 根据 targetSrt 构建富文本 Span
            for (int i = 0; i < (int)phrase.targetSrt.size(); ++i) {
                const auto& word_data = phrase.targetSrt[i];
                if (i == active_word_idx) {
                    label->add_span({word_data.word, highlight_style});
                } else {
                    label->add_span({word_data.word, normal_style});
                }
            }

            last_phrase_idx = active_phrase_idx;
            last_word_idx = active_word_idx;
        }
    }

    void update_logic_for_slider(double dt) {
        if (active_phrase_idx == -1 || active_word_idx == -1) {
            return;
        }

        label->show();
        auto& phrase = subtitles[active_phrase_idx];

        // 句子切换
        if (active_phrase_idx != last_phrase_idx) {
            last_phrase_idx = active_phrase_idx;
            rebuild_contents(phrase);
            label->calc_minimum_size();
            label->adjust_layout();
            calculate_word_rects(phrase);
        }

        // 更新活跃词和进度
        active_word_idx = -1;
        double progress = 0;
        for (int i = 0; i < (int)phrase.targetSrt.size(); ++i) {
            auto& w = phrase.targetSrt[i];
            if (current_time >= w.start && current_time <= w.end) {
                active_word_idx = i;
                progress = (current_time - w.start) / (w.end - w.start);
                break;
            }
        }

        if (active_word_idx != -1) {
            target_slider_rect = phrase.targetSrt[active_word_idx].rect.dilate(4);
        }
        // No lerp for the first word.
        if (active_word_idx == 0) {
            current_slider_rect = target_slider_rect;
        } else {
            current_slider_rect = lerp_rect(current_slider_rect, target_slider_rect, 10.0f * (float)dt);
        }
    }

    void rebuild_contents(const SubtitlePhrase& phrase) {
        label->clear_spans();

        TextStyle base;
        base.color = (sub_type == SubtitleType::Karaoke) ? karaoke_style.unreached : ColorU::white();
        base.shadow_color = ColorU::black();
        base.shadow_radius = 4;

        TextStyle high = base;
        if (sub_type == SubtitleType::Karaoke)
            high.color = karaoke_style.reached;
        else if (sub_type == SubtitleType::Basic)
            high = basic_style.highlight;

        for (const auto& w : phrase.targetSrt) {
            label->add_span({w.word, base});
        }
    }

    void update_logic_for_karaoke(double dt) {
        if (active_phrase_idx == -1) {
            return;
        }

        const auto& phrase = subtitles[active_phrase_idx];

        // 1. 句子切换时重建内容
        if (active_phrase_idx != last_phrase_idx) {
            rebuild_contents(phrase);
            label->adjust_layout();
            last_phrase_idx = active_phrase_idx;
        }

        // 2. 找到当前正在唱的词和进度
        int w_idx = -1;
        double progress = 0;
        for (int i = 0; i < (int)phrase.targetSrt.size(); ++i) {
            auto& w = phrase.targetSrt[i];
            if (current_time >= w.start && current_time <= w.end) {
                w_idx = i;
                progress = (current_time - w.start) / (w.end - w.start);
                break;
            }
        }

        // 3. 核心修复：逐帧更新 Label 中每个字符的样式
        auto& glyphs = label->get_glyphs();
        size_t current_char_offset = 0;
        int glyph_ptr = 0;

        for (int i = 0; i < (int)phrase.targetSrt.size(); ++i) {
            const auto& word_data = phrase.targetSrt[i];
            std::u32string w32;
            utf8_to_utf32(word_data.word, w32);
            size_t word_len = w32.size();

            while (glyph_ptr < (int)glyphs.size()) {
                auto& g = glyphs[glyph_ptr];
                if (g.start >= (int)(current_char_offset + word_len)) break;

                g.style.stroke_color = karaoke_style.normal.stroke_color;
                g.style.stroke_width = karaoke_style.normal.stroke_width;

                // 设置渐变参数
                if (i < w_idx) {
                    // 已唱完：全变色，禁用渐变
                    g.style.color = karaoke_style.reached;
                    g.style.karaoke_progress = -1.0f;
                } else if (i == w_idx) {
                    // 正在唱：设置渐变和进度
                    g.style.color = karaoke_style.unreached;
                    g.style.karaoke_reached_color = karaoke_style.reached;
                    g.style.karaoke_progress = (float)progress;
                } else {
                    // 未唱到：保持原色，禁用渐变
                    g.style.color = karaoke_style.unreached;
                    g.style.karaoke_progress = -1.0f;
                }
                glyph_ptr++;
            }
            current_char_offset += word_len;
        }
    }

    void calculate_word_rects(SubtitlePhrase& phrase) {
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

    void load_default_subtitles_data() {
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

    std::shared_ptr<Label> label;
    std::vector<SubtitlePhrase> subtitles;
    double current_time = 0;
    int last_phrase_idx = -1;
    int last_word_idx = -1;

    int active_phrase_idx = -1;
    int active_word_idx = -1;
    RectF current_slider_rect;
    RectF target_slider_rect;
    RectF karaoke_clip_rect;

    std::shared_ptr<Label> debug_label_;
};

int main() {
    App app({1280, 720}, true);

    // 添加字幕显示节点到场景树
    app.get_tree_root()->add_child(std::make_shared<SubtitleNode>());

    app.main_loop();

    return EXIT_SUCCESS;
}
