#include "subtitle_style_config.h"

#include <nlohmann/json.hpp>

// 颜色转换辅助
ColorU hex_to_color(const std::string& hex) {
    if (hex.empty() || hex[0] != '#') return ColorU::transparent_black();

    uint32_t r, g, b, a = 255;
    if (hex.length() == 9) { // #RRGGBBAA
        std::sscanf(hex.c_str(), "#%02x%02x%02x%02x", &r, &g, &b, &a);
    } else if (hex.length() == 7) { // #RRGGBB
        std::sscanf(hex.c_str(), "#%02x%02x%02x", &r, &g, &b);
    } else {
        return ColorU::transparent_black();
    }
    return ColorU(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b), static_cast<uint8_t>(a));
}

std::optional<SubtitleAnimation> get_subtitle_animation(const SubtitleAnimationType type) {
    SubtitleAnimation anim;

    switch (type) {
        case SubtitleAnimationType::None:
            return {};
        case SubtitleAnimationType::FadeIn: {
            anim.alpha_start = 0.0f;
            anim.alpha_end = 1.0f;
        } break;
        case SubtitleAnimationType::SlideUp: {
            anim.alpha_start = 0.0f;
            anim.alpha_end = 1.0f;
            anim.transform_start = Transform2::from_translation({0, 20});
            anim.transform_end = Transform2();
        } break;
        case SubtitleAnimationType::SlideDown: {
            anim.alpha_start = 0.0f;
            anim.alpha_end = 1.0f;
            anim.transform_start = Transform2::from_translation({0, -20});
            anim.transform_end = Transform2();
        } break;
        case SubtitleAnimationType::ScaleUp: {
            anim.alpha_start = 0.0f;
            anim.alpha_end = 1.0f;
            anim.transform_start = Transform2::from_scale({0.5, 0.5});
            anim.transform_end = Transform2();
        } break;
        case SubtitleAnimationType::ScaleDown: {
            anim.alpha_start = 0.0f;
            anim.alpha_end = 1.0f;
            anim.transform_start = Transform2::from_scale({1.25, 1.25});
            anim.transform_end = Transform2();
        } break;
        case SubtitleAnimationType::Max:
            return {};
    }

    return {anim};
}

void parse_text_style(const nlohmann::json& j, TextStyle& style) {
    if (j.contains("color")) style.color = hex_to_color(j["color"]);
    if (j.contains("font_size")) style.font_size = j["font_size"];
    if (j.contains("italic")) style.italic = j["italic"];
    if (j.contains("bold")) style.bold = j["bold"];
    if (j.contains("stroke_color")) style.stroke_color = hex_to_color(j["stroke_color"]);
    if (j.contains("stroke_width")) style.stroke_width = j["stroke_width"];
    if (j.contains("shadow_color")) style.shadow_color = hex_to_color(j["shadow_color"]);
    if (j.contains("shadow_radius")) style.shadow_radius = j["shadow_radius"];
    if (j.contains("shadow_offset")) {
        style.shadow_offset = Vec2F(j["shadow_offset"][0], j["shadow_offset"][1]);
    }
}

void from_json(const nlohmann::json& j, SubtitleStyleConfig& cfg) {
    // 1. 解析 Font
    if (j.contains("font")) {
        auto& jf = j["font"];
        cfg.font_config.font_path = jf.value("path", "");
        if (jf.contains("size")) {
            if (jf["size"].is_string()) cfg.font_config.font_size = jf["size"];
            else cfg.font_config.font_size = std::to_string(jf["size"].get<int>());
        }
        cfg.font_config.letter_spacing = jf.value("letter_spacing", 0.0f);
        cfg.font_config.line_spacing = jf.value("line_spacing", 0.0f);
    }

    // 2. 解析 Normal Style
    if (j.contains("normal_style")) {
        parse_text_style(j["normal_style"], cfg.normal);
    }

    // 3. 解析全局背景
    if (j.contains("global_background")) {
        auto& jbg = j["global_background"];
        cfg.background_color = hex_to_color(jbg.value("color", "#00000000"));
        cfg.background_padding = jbg.value("padding", 0.0f);
        cfg.background_corner_radius = jbg.value("corner_radius", 0.0f);
    }

    // 4. 解析 Highlight
    if (j.contains("highlight")) {
        std::string h_type = j["highlight"].value("type", "Basic");
        auto& params = j["highlight"]["params"];

        if (h_type == "Basic") {
            cfg.highlight_type = SubtitleHighlightType::Basic;
            SubtitleHighlightBasic b;
            b.highlight = cfg.normal; // 默认继承 normal
            parse_text_style(params, b.highlight);
            cfg.highlight_basic = b;
        } else if (h_type == "Slider") {
            cfg.highlight_type = SubtitleHighlightType::Slider;
            SubtitleHighlightSlider s;
            s.slider = StyleBox::from_empty();
            if (params.contains("bg_color")) s.slider.bg_color = hex_to_color(params["bg_color"]);
            if (params.contains("border_color")) s.slider.border_color = hex_to_color(params["border_color"]);
            s.slider.border_width = params.value("border_width", 0.0f);
            s.slider.corner_radius = params.value("corner_radius", 0.0f);
            cfg.highlight_slider = s;
        } else if (h_type == "Karaoke") {
            cfg.highlight_type = SubtitleHighlightType::Karaoke;
            SubtitleHighlightKaraoke k;
            k.reached = hex_to_color(params.value("reached_color", "#FFFFFFFF"));
            k.unreached = hex_to_color(params.value("unreached_color", "#FFFFFFFF"));
            cfg.highlight_karaoke = k;
        }
    }

    // 5. 解析 Animation (仅暴露 Type)
    if (j.contains("animation")) {
        std::string a_type_str = "";
        if (j["animation"].is_string()) {
            a_type_str = j["animation"];
        } else if (j["animation"].is_object()) {
            a_type_str = j["animation"].value("type", "None");
        }

        SubtitleAnimationType a_type = SubtitleAnimationType::None;
        if (a_type_str == "FadeIn") a_type = SubtitleAnimationType::FadeIn;
        else if (a_type_str == "SlideUp") a_type = SubtitleAnimationType::SlideUp;
        else if (a_type_str == "SlideDown") a_type = SubtitleAnimationType::SlideDown;
        else if (a_type_str == "ScaleUp") a_type = SubtitleAnimationType::ScaleUp;
        else if (a_type_str == "ScaleDown") a_type = SubtitleAnimationType::ScaleDown;

        cfg.animation_type = a_type;
        cfg.animation = get_subtitle_animation(a_type);
    }
}
