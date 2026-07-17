#pragma once

#include "../node_ui.h"
#include "nodes/ui/label.h"
#include "subtitle_style_config.h"

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

// 2. 自定义字幕显示节点
class Subtitle : public NodeUi {
public:
    Subtitle();

    std::vector<SubtitleStyleConfig> subtitle_config_presets;
    uint32_t preset_idx = 0;
    SubtitleStyleConfig active_subtitle_config;

    void custom_ready() override;

    void custom_input(InputEvent& event) override;

    bool load_from_file(const std::string& path);

    void custom_update(double dt) override;

    void draw() override;

    void set_font(std::shared_ptr<Font> font);

private:
    std::vector<RectF> get_line_rects();

    void setup_default_styles();

    void update_phrase_word_idx();

    void refresh_subtitle_display(double dt);

    void update_slider_logic(double dt);

    void calculate_word_rects(SubtitlePhrase& phrase);

    void load_default_subtitles_data();

    std::shared_ptr<Label> label;
    std::vector<SubtitlePhrase> subtitles;
    double current_time = 0;
    int last_phrase_idx = -1;
    int last_word_idx = -1;

    int active_phrase_idx = -1;
    int active_word_idx = -1;
    RectF current_slider_rect;
    RectF target_slider_rect;

    std::shared_ptr<Label> debug_label_;
};

} // namespace vecgui
