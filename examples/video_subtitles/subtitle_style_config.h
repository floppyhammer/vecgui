#pragma once

#include "app.h"

using namespace vecgui;

/// Once animation exits, unreached words should be invisible.
enum class SubtitleAnimationType {
    None = 0,
    FadeIn = 1,
    SlideUp = 2,
    SlideDown = 3,
    ScaleUp = 4,
    ScaleDown = 5,
    Max,
};

struct SubtitleAnimation {
    float alpha_start;
    float alpha_end;
    Transform2 transform_start;
    Transform2 transform_end;
};

struct FontConfig {
    std::string font_path;
    std::string font_size;
    float letter_spacing;
    float line_spacing;
};

// 1. 字幕样式类型
enum class SubtitleHighlightType {
    Basic,
    Slider,  // 滑块背景随文字滑动
    Karaoke, // 逐字进度染色效果
};

struct SubtitleHighlightBasic {
    TextStyle highlight;
};

struct SubtitleHighlightSlider {
    StyleBox slider;
};

struct SubtitleHighlightKaraoke {
    ColorU reached;
    ColorU unreached;
};

struct SubtitleStyleConfig {
    FontConfig font_config;
    TextStyle normal;

    // 全局字幕背景 (每行)
    ColorU background_color = ColorU::transparent_black();
    float background_corner_radius = 0;
    float background_padding = 0; // 外扩距离

    SubtitleHighlightType highlight_type;
    std::optional<SubtitleHighlightBasic> highlight_basic;
    std::optional<SubtitleHighlightSlider> highlight_slider;
    std::optional<SubtitleHighlightKaraoke> highlight_karaoke;

    SubtitleAnimationType animation_type;
    std::optional<SubtitleAnimation> animation;
};

std::optional<SubtitleAnimation> get_subtitle_animation(const SubtitleAnimationType type);
