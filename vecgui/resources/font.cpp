#include "font.h"

#include <string>
#include <vector>

#include "../common/utils.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

// For debugging glyph bitmap
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#ifndef VECGUI_USE_FRIBIDI
    #ifdef _WIN32
        // With Windows 10 Fall Creators Update and later, you can just include the single header <icu.h>.
        // See https://learn.microsoft.com/en-us/windows/win32/intl/international-components-for-unicode--icu-
        #include <icu.h>
    #elif __linux__
        #include <unicode/ubidi.h>
        #include <unicode/ubrk.h>
        #include <unicode/uclean.h>
        #include <unicode/udata.h>
        #include <unicode/uscript.h>
        #include <unicode/utypes.h>
    #endif
#else
    #include <fribidi.h>
#endif

#include <hb-ot.h>
#include <hb.h>

#include <optional>

#include "../servers/engine.h"
#include "../servers/text_server.h"
#include "default_resource.h"

namespace vecgui {

hb_script_t to_harfbuzz_script(Script script) {
    switch (script) {
        case Script::Arabic: {
            return HB_SCRIPT_ARABIC;
        }
        case Script::Hebrew: {
            return HB_SCRIPT_HEBREW;
        }
        case Script::Han: {
            return HB_SCRIPT_HAN;
        }
        case Script::Hangul: {
            return HB_SCRIPT_HANGUL;
        }
        case Script::Bengali: {
            return HB_SCRIPT_BENGALI;
        }
        case Script::Devanagari: {
            return HB_SCRIPT_DEVANAGARI;
        }
        case Script::Thai: {
            return HB_SCRIPT_THAI;
        }
        case Script::Hiragana: {
            return HB_SCRIPT_HIRAGANA;
        }
        case Script::Katakana: {
            return HB_SCRIPT_KATAKANA;
        }
        default: {
            return HB_SCRIPT_COMMON;
        }
    }
}

std::vector<std::pair<Script, Pathfinder::Range>> get_text_script(const std::u32string &utf32_text) {
    std::vector<Script> scripts;

    for (auto &codepoint : utf32_text) {
        if (codepoint >= 0x0600 && codepoint <= 0x06FF) {
            scripts.push_back(Script::Arabic);
        } else if (codepoint >= 0x0981 && codepoint <= 0x09FB) {
            scripts.push_back(Script::Bengali);
        } else if (codepoint >= 0x0901 && codepoint <= 0x097F) {
            scripts.push_back(Script::Devanagari);
        } else if (codepoint >= 0x0590 && codepoint <= 0x05FF) {
            scripts.push_back(Script::Hebrew);
        } else if ((codepoint >= 0xAC00 && codepoint <= 0xD7AF) || (codepoint >= 0x1100 && codepoint <= 0x11FF) ||
                   (codepoint >= 0x3130 && codepoint <= 0x318F) || (codepoint >= 0xA960 && codepoint <= 0xA97F) ||
                   (codepoint >= 0xD7B0 && codepoint <= 0xD7FF)) {
            scripts.push_back(Script::Hangul);
        } else if ((codepoint >= 0x4E00 && codepoint <= 0x9FFF) || (codepoint >= 0x3400 && codepoint <= 0x4DBF) ||
                   (codepoint >= 0x20000 && codepoint <= 0x2A6DF) || (codepoint >= 0xF900 && codepoint <= 0xFAFF) ||
                   (codepoint >= 0x2F800 && codepoint <= 0x2FA1F) || (codepoint >= 0x3000 && codepoint <= 0x303F) ||
                   (codepoint >= 0xFF00 && codepoint <= 0xFFEF)) {
            scripts.push_back(Script::Han);
        } else if (codepoint >= 0x3040 && codepoint <= 0x309F) {
            scripts.push_back(Script::Hiragana);
        } else if (codepoint >= 0x30A0 && codepoint <= 0x30FF) {
            scripts.push_back(Script::Katakana);
        } else if (codepoint >= 0x0E00 && codepoint <= 0x0E7F) {
            scripts.push_back(Script::Thai);
        } else {
            scripts.push_back(Script::Common);
        }
    }

    std::vector<std::pair<Script, Pathfinder::Range>> script_groups;
    auto current_script = scripts.front();
    uint32_t current_codepoint_start = 0;
    for (uint32_t idx = 0; idx < scripts.size(); idx++) {
        const auto &s = scripts[idx];

        if (s != current_script) {
            script_groups.emplace_back(current_script, Pathfinder::Range{current_codepoint_start, idx});

            current_script = s;
            current_codepoint_start = idx;
        }
    }

    script_groups.emplace_back(current_script, Pathfinder::Range{current_codepoint_start, scripts.size()});

    return script_groups;
}

bool glyphs_exist_in_font(std::u32string codepoints, Font *font) {
    assert(font != nullptr);

    for (const auto &c : codepoints) {
        // Skip line breaks.
        if (c == 0x000A) {
            continue;
        }
        if (font->find_glyph_index_by_codepoint(c) == 0) {
            return false;
        }
    }
    return true;
}

struct HarfBuzzData {
    hb_blob_t *blob{};
    hb_face_t *face{};
    hb_font_t *font{};
    std::vector<ColorU> palette;

    HarfBuzzData() = default;

    explicit HarfBuzzData(const std::vector<char> &bytes) {
        // We need to keep bytes.data() valid for HarfBuzz to work properly.
        blob = hb_blob_create(bytes.data(), bytes.size(), HB_MEMORY_MODE_READONLY, nullptr, nullptr);
        face = hb_face_create(blob, 0);
        font = hb_font_create(face);

        // Crucial: Initialize OT functions for the font.
        hb_ot_font_set_funcs(font);

        // Check support.
        bool has_layers = hb_ot_color_has_layers(face);
        bool has_palettes = hb_ot_color_palette_get_count(face) > 0;
        bool has_paint = hb_ot_color_has_paint(face);

        printf("Font Analysis: COLR_v0=%d, Palettes=%d, COLR_v1=%d\n", has_layers, has_palettes, has_paint);

        // Load default palette (0).
        unsigned int count = hb_ot_color_palette_get_colors(face, 0, 0, nullptr, nullptr);
        if (count > 0) {
            std::vector<hb_color_t> hb_colors(count);
            hb_ot_color_palette_get_colors(face, 0, 0, &count, hb_colors.data());
            palette.reserve(count);
            for (auto c : hb_colors) {
                palette.push_back(
                    ColorU(hb_color_get_red(c), hb_color_get_green(c), hb_color_get_blue(c), hb_color_get_alpha(c)));
            }
        }
    }

    ~HarfBuzzData() {
        if (font) {
            hb_font_destroy(font);
        }
        if (face) {
            hb_face_destroy(face);
        }
        if (blob) {
            hb_blob_destroy(blob);
        }
    }
};

std::shared_ptr<Font> Font::from_file(const GuiContext *context, const std::string &path) {
#ifndef __ANDROID__
    auto bytes = Pathfinder::load_file_as_bytes(path);
#else
    auto bytes = Pathfinder::load_asset(context->engine->asset_manager, path);
#endif

    return from_memory(bytes);
}

std::shared_ptr<Font> Font::from_memory(const std::vector<char> &bytes) {
    if (bytes.empty()) {
        return nullptr;
    }

    auto font = std::make_shared<Font>();
    font->font_data = bytes;

    const auto byte_size = font->font_data.size() * sizeof(unsigned char);

    font->stbtt_buffer = static_cast<unsigned char *>(malloc(byte_size));
    memcpy(font->stbtt_buffer, font->font_data.data(), byte_size);

    // Prepare font info.
    font->stbtt_info = new stbtt_fontinfo;
    if (!stbtt_InitFont(font->stbtt_info, font->stbtt_buffer, 0)) {
        Logger::error("Failed to prepare font info!", "vecgui");
        return nullptr;
    }

    font->harfbuzz_data = std::make_shared<HarfBuzzData>(font->font_data);

    return font;
}

Font::~Font() {
    if (stbtt_buffer) {
        free(stbtt_buffer);
    }

    delete stbtt_info;
}

float Font::update_metrics(uint32_t size, float &ascent, float &descent) {
    // Calculate font scaling.
    float scale = stbtt_ScaleForPixelHeight(stbtt_info, (float)size);

    // The origin is baseline and the Y axis points upward.
    // So, ascent is usually positive, and descent negative.
    int unscaled_ascent;
    int unscaled_descent;
    int unscaled_line_gap;
    stbtt_GetFontVMetrics(stbtt_info, &unscaled_ascent, &unscaled_descent, &unscaled_line_gap);

    // Take scale into account.
    ascent = float(unscaled_ascent) * scale;
    descent = float(unscaled_descent) * scale;

    return scale;
}

struct PaintContext {
    const Font *font;
    float scale;
    std::vector<GlyphLayer> *layers;
    std::vector<uint16_t> clip_glyph_stack;
};

static void paint_color_callback(
    hb_paint_funcs_t *funcs, void *paint_data, hb_bool_t is_foreground, hb_color_t color, void *user_data) {
    auto *ctx = static_cast<PaintContext *>(paint_data);
    if (ctx->clip_glyph_stack.empty()) return;

    // In COLR v1, this is often used to fill the current clip glyph.
    GlyphLayer layer;
    layer.index = ctx->clip_glyph_stack.back();
    if (is_foreground) {
        layer.fill = ColorU::transparent_black();
    } else {
        layer.fill = ColorU(
            hb_color_get_red(color), hb_color_get_green(color), hb_color_get_blue(color), hb_color_get_alpha(color));
    }
    layer.path = ctx->font->get_glyph_path(layer.index, ctx->scale);
    ctx->layers->push_back(layer);
}

static void paint_fill_glyph_callback(hb_paint_funcs_t *funcs,
                                      void *paint_data,
                                      hb_codepoint_t glyph_id,
                                      hb_font_t *font,
                                      hb_bool_t is_foreground,
                                      hb_color_t color,
                                      void *user_data) {
    auto *ctx = static_cast<PaintContext *>(paint_data);
    GlyphLayer layer;
    layer.index = glyph_id;
    if (is_foreground) {
        layer.fill = ColorU::transparent_black();
    } else {
        layer.fill = ColorU(
            hb_color_get_red(color), hb_color_get_green(color), hb_color_get_blue(color), hb_color_get_alpha(color));
    }
    layer.path = ctx->font->get_glyph_path(layer.index, ctx->scale);
    ctx->layers->push_back(layer);
}

static void push_clip_glyph_callback(
    hb_paint_funcs_t *funcs, void *paint_data, hb_codepoint_t glyph_id, hb_font_t *font, void *user_data) {
    auto *ctx = static_cast<PaintContext *>(paint_data);
    ctx->clip_glyph_stack.push_back((uint16_t)glyph_id);
}

static void pop_clip_callback(hb_paint_funcs_t *funcs, void *paint_data, void *user_data) {
    auto *ctx = static_cast<PaintContext *>(paint_data);
    if (!ctx->clip_glyph_stack.empty()) {
        ctx->clip_glyph_stack.pop_back();
    }
}

static Pathfinder::Gradient convert_hb_color_line(hb_color_line_t *color_line) {
    unsigned int count = hb_color_line_get_color_stops(color_line, 0, nullptr, nullptr);
    std::vector<hb_color_stop_t> hb_stops(count);
    hb_color_line_get_color_stops(color_line, 0, &count, hb_stops.data());

    Pathfinder::Gradient gradient;
    for (const auto &s : hb_stops) {
        ColorU stop_color(hb_color_get_red(s.color),
                          hb_color_get_green(s.color),
                          hb_color_get_blue(s.color),
                          hb_color_get_alpha(s.color));
        gradient.add_color_stop(stop_color, s.offset);
    }

    auto extend = hb_color_line_get_extend(color_line);
    switch (extend) {
        case HB_PAINT_EXTEND_REPEAT:
            gradient.wrap = Pathfinder::GradientWrap::Repeat;
            break;
        case HB_PAINT_EXTEND_REFLECT:
            // Pathfinder might not support reflect, fallback to repeat or clamp
            gradient.wrap = Pathfinder::GradientWrap::Repeat;
            break;
        default:
            gradient.wrap = Pathfinder::GradientWrap::Clamp;
            break;
    }

    return gradient;
}

static void paint_linear_gradient_callback(hb_paint_funcs_t *funcs,
                                           void *paint_data,
                                           hb_color_line_t *color_line,
                                           float x0,
                                           float y0,
                                           float x1,
                                           float y1,
                                           float x2,
                                           float y2,
                                           void *user_data) {
    auto *ctx = static_cast<PaintContext *>(paint_data);
    if (ctx->clip_glyph_stack.empty()) return;

    auto gradient = convert_hb_color_line(color_line);

    // Convert to Pathfinder Linear Geometry.
    // Note: HarfBuzz uses Y-down, but Font class handles flip in get_glyph_path.
    // For gradients, we need to match the font's coordinate system.
    Vec2F p0(x0 * ctx->scale, y0 * -ctx->scale);
    Vec2F p1(x1 * ctx->scale, y1 * -ctx->scale);
    gradient.geometry = Pathfinder::GradientLinear{Pathfinder::LineSegmentF(p0, p1)};

    GlyphLayer layer;
    layer.index = ctx->clip_glyph_stack.back();
    layer.fill = gradient;
    layer.path = ctx->font->get_glyph_path(layer.index, ctx->scale);
    ctx->layers->push_back(layer);
}

static void paint_radial_gradient_callback(hb_paint_funcs_t *funcs,
                                           void *paint_data,
                                           hb_color_line_t *color_line,
                                           float x0,
                                           float y0,
                                           float r0,
                                           float x1,
                                           float y1,
                                           float r1,
                                           void *user_data) {
    auto *ctx = static_cast<PaintContext *>(paint_data);
    if (ctx->clip_glyph_stack.empty()) return;

    auto gradient = convert_hb_color_line(color_line);

    Vec2F c0(x0 * ctx->scale, y0 * -ctx->scale);
    Vec2F c1(x1 * ctx->scale, y1 * -ctx->scale);
    float radius = r1 * ctx->scale; // HarfBuzz r1 is the outer radius.

    gradient.geometry =
        Pathfinder::GradientRadial{Pathfinder::LineSegmentF(c0, c1), Vec2F(r0 * ctx->scale, r1 * ctx->scale)};

    GlyphLayer layer;
    layer.index = ctx->clip_glyph_stack.back();
    layer.fill = gradient;
    layer.path = ctx->font->get_glyph_path(layer.index, ctx->scale);
    ctx->layers->push_back(layer);
}

void Font::populate_glyph_color_layers(Glyph &glyph, float scale) const {
    if (!harfbuzz_data || !harfbuzz_data->face) return;

    // Try COLR v0 first (backward compatibility).
    unsigned int layer_count = hb_ot_color_glyph_get_layers(harfbuzz_data->face, glyph.index, 0, nullptr, nullptr);
    if (layer_count > 0) {
        std::vector<hb_ot_color_layer_t> hb_layers(layer_count);
        hb_ot_color_glyph_get_layers(harfbuzz_data->face, glyph.index, 0, &layer_count, hb_layers.data());
        glyph.layers.clear();
        for (const auto &l : hb_layers) {
            GlyphLayer layer;
            layer.index = l.glyph;
            if (l.color_index != 0xFFFF && l.color_index < harfbuzz_data->palette.size()) {
                layer.fill = harfbuzz_data->palette[l.color_index];
            } else {
                layer.fill = ColorU::transparent_black();
            }
            layer.path = get_glyph_path(layer.index, scale);
            glyph.layers.push_back(layer);
        }
        glyph.emoji = true;
        return;
    }

    // Try COLR v1 using the Paint API.
    if (hb_ot_color_has_paint(harfbuzz_data->face)) {
        static hb_paint_funcs_t *paint_funcs = nullptr;
        if (!paint_funcs) {
            paint_funcs = hb_paint_funcs_create();
            hb_paint_funcs_set_color_func(paint_funcs, paint_color_callback, nullptr, nullptr);
            hb_paint_funcs_set_fill_glyph_func(paint_funcs, paint_fill_glyph_callback, nullptr, nullptr);
            hb_paint_funcs_set_push_clip_glyph_func(paint_funcs, push_clip_glyph_callback, nullptr, nullptr);
            hb_paint_funcs_set_pop_clip_func(paint_funcs, pop_clip_callback, nullptr, nullptr);
            hb_paint_funcs_set_linear_gradient_func(paint_funcs, paint_linear_gradient_callback, nullptr, nullptr);
            hb_paint_funcs_set_radial_gradient_func(paint_funcs, paint_radial_gradient_callback, nullptr, nullptr);
            hb_paint_funcs_make_immutable(paint_funcs);
        }

        glyph.layers.clear();
        PaintContext ctx{this, scale, &glyph.layers, {}};

        hb_font_paint_glyph(harfbuzz_data->font, glyph.index, paint_funcs, &ctx, 0, 0);

        if (!glyph.layers.empty()) {
            glyph.emoji = true;
        }
    }
}

Pathfinder::Path2d Font::get_glyph_path(uint16_t glyph_index, float scale) const {
    Pathfinder::Path2d path;

    stbtt_vertex *vertices{};
    int num_vertices = stbtt_GetGlyphShape(stbtt_info, glyph_index, &vertices);

    // Glyph has no shape (e.g. Space).
    if (vertices == nullptr) {
        if (glyph_index == 0) {
            // Manually generate a "tofu" (missing glyph) box if the font doesn't provide one.
            float ascent, descent, width;
            get_tofu_metrics(scale, ascent, descent, width);

            path.move_to(0, -ascent);
            path.line_to(width, -ascent);
            path.line_to(width, -descent);
            path.line_to(0, -descent);
            path.close_path();
        }
        return path;
    }

    for (int i = 0; i < num_vertices; i++) {
        auto &v = vertices[i];

        switch (v.type) {
            case STBTT_vmove: {
                // Close the last contour in the outline (if there's any).
                path.close_path();
                path.move_to(v.x * scale, v.y * -scale);
            } break;
            case STBTT_vline: {
                path.line_to(v.x * scale, v.y * -scale);
            } break;
            case STBTT_vcurve: {
                path.quadratic_to(v.cx * scale, v.cy * -scale, v.x * scale, v.y * -scale);
            } break;
            case STBTT_vcubic: {
                path.cubic_to(v.cx * scale, v.cy * -scale, v.cx1 * scale, v.cy1 * -scale, v.x * scale, v.y * -scale);
            } break;
        }
    }

    // Close the last contour in the outline.
    path.close_path();

    stbtt_FreeShape(stbtt_info, vertices);

    return path;
}

#ifndef VECGUI_USE_FRIBIDI

// Not font fallback when using ICU.

void Font::get_glyphs(TextServer *text_server,
                      const std::string &text,
                      uint32_t font_size,
                      std::vector<Glyph> &glyphs,
                      std::vector<Line> &paragraphs) {
    glyphs.clear();
    paragraphs.clear();

    #ifdef ICU_STATIC_DATA
    static bool icu_data_loaded = false;
    if (!icu_data_loaded) {
        UErrorCode err = U_ZERO_ERROR;
        u_init(&err); // Do not check for errors, since we only load part of the data.
        icu_data_loaded = true;
    }
    #else
    // Load data manually.
    #endif

    uint32_t units_per_em = hb_face_get_upem(harfbuzz_data->face);

    // Note: don't use icu::UnicodeString, it doesn't work. Use plain UChar* instead.

    std::u16string text_u16;
    utf8_to_utf16(text, text_u16);

    const UChar *uchar_data = text_u16.c_str();
    const int32_t uchar_count = text_u16.length();

    // Bidi for the whole text (paragraphs).
    UBiDi *para_bidi = ubidi_open();
    // Bidi for a paragraph (lines).
    // This would a child bidi of para_bidi. The order to destroy them matters.
    UBiDi *line_bidi = ubidi_open();

    UErrorCode error_code = U_ZERO_ERROR;

    do {
        // Paragraphs are seperated by line breaks.
        //        std::cout << "Paragraphs: " << text << std::endl;

        // Set paragraphs.
        ubidi_setPara(para_bidi, uchar_data, uchar_count, UBIDI_DEFAULT_LTR, nullptr, &error_code);
        if (!U_SUCCESS(error_code)) {
            Logger::error("ubidi_setPara() failed!", "vecgui");
            break;
        }

        int32_t para_count = ubidi_countParagraphs(para_bidi);

        // Go through paragraphs.
        for (int32_t para_index = 0; para_index < para_count; para_index++) {
            // Paragraph start and end in the whole text. Unit: u16char.
            int32_t para_start, para_end;
            UBiDiLevel para_level;
            ubidi_getParagraphByIndex(para_bidi, para_index, &para_start, &para_end, &para_level, &error_code);

            if (!U_SUCCESS(error_code)) {
                Logger::error("ubidi_getParagraphByIndex() failed!", "vecgui");
                break;
            }

            //            std::string para_text = to_utf8(text_u16.substr(para_start, para_end));
            //            std::cout << "Paragraph text: " << para_text << std::endl;
            //            std::cout << "Paragraph range: " << para_start << " -> " << para_end << std::endl;

            // Set a paragraph (lines).
            ubidi_setLine(para_bidi, para_start, para_end, line_bidi, &error_code);
            if (!U_SUCCESS(error_code)) {
                Logger::error("ubidi_setLine failed!", "vecgui");
                break;
            }

            bool para_is_rtl = false;

            // The width of the paragraph in a single line.
            float para_width = 0;

            // The first glyph in the new paragraph.
            size_t para_glyph_start = glyphs.size();

            // Get run count in the current paragraph.
            int32_t run_count = ubidi_countRuns(line_bidi, &error_code);

            // Go through runs.
            for (int32_t run_index = 0; run_index < run_count; run_index++) {
                // Run start and end in the paragraph. Unit: u16char.
                int32_t logical_start, length;
                UBiDiDirection dir = ubidi_getVisualRun(line_bidi, run_index, &logical_start, &length);

                bool run_is_rtl = dir == UBIDI_RTL;

                para_is_rtl |= run_is_rtl;

                // Get run text from the whole text.
                std::u16string run_text_u16 = text_u16.substr(para_start + logical_start, length);

                std::string run_text = utf16_to_utf8(run_text_u16);

                std::u32string run_text_u32;
                utf8_to_utf32(run_text, run_text_u32);

                //                std::cout << "Visual run in paragraph: \t" << run_index << "\t" << run_is_rtl << "\t"
                //                << logical_start
                //                          << '\t' << length << '\t' << run_text << std::endl;

                auto run_script = get_text_script(run_text_u32).front().first;

                Font *font_to_use = this;
                if (allow_fallback && !glyphs_exist_in_font(run_text_u32, this)) {
                    auto script_font = text_server->get_font_for_script(run_script);
                    if (!script_font) {
                        script_font = text_server->get_font_for_script(Script::Common);
                    }

                    if (script_font) {
                        font_to_use = script_font.get();
                    }
                }

                float ascent, descent;
                float scale = font_to_use->update_metrics(font_size, ascent, descent);

                // Buffers are sequences of Unicode characters that use the same font
                // and have the same text direction, script, and language.
                hb_buffer_t *hb_buffer = hb_buffer_create();

                // Item offset and length should represent a specific run.
                hb_buffer_add_utf16(
                    hb_buffer, reinterpret_cast<const uint16_t *>(uchar_data), -1, para_start + logical_start, length);

                hb_buffer_set_direction(hb_buffer, run_is_rtl ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
                hb_buffer_set_script(hb_buffer, to_harfbuzz_script(run_script));

                hb_shape(font_to_use->harfbuzz_data->font, hb_buffer, nullptr, 0);

                unsigned int glyph_count;
                hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(hb_buffer, &glyph_count);
                hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(hb_buffer, &glyph_count);

                std::vector<hb_glyph_info_t> debug_glyph_info(glyph_count);
                for (int i = 0; i < glyph_count; i++) {
                    debug_glyph_info[i] = glyph_info[i];
                }

                // Shaped glyph positions will always be in one line (regardless of line breaks).
                for (int i = 0; i < glyph_count; i++) {
                    auto &info = glyph_info[i];
                    auto &pos = glyph_pos[i];

                    // Cluster unit is u16char, so it should be worked with std::u16string instead of std::string.
                    std::optional<Pathfinder::Range> current_cluster;
                    if (!run_is_rtl) {
                        if (i < glyph_count - 1) {
                            // Multiple glyphs may share the same cluster.
                            for (int j = 1; i + j < glyph_count; j++) {
                                if (info.cluster != glyph_info[i + j].cluster) {
                                    current_cluster = {info.cluster, glyph_info[i + j].cluster};
                                    break;
                                }
                            }
                        }
                        if (!current_cluster.has_value()) {
                            current_cluster = {info.cluster, (unsigned long long)(para_start + logical_start + length)};
                        }
                    } else {
                        if (i > 0) {
                            // Multiple glyphs may share the same cluster.
                            for (int j = 1; i - j >= 0; j++) {
                                if (info.cluster != glyph_info[i - j].cluster) {
                                    current_cluster = {info.cluster, glyph_info[i - j].cluster};
                                    break;
                                }
                            }
                        }
                        if (!current_cluster.has_value()) {
                            current_cluster = {info.cluster, (unsigned long long)(para_start + logical_start + length)};
                        }
                    }

                    std::u16string glyph_text_u16 = text_u16.substr(current_cluster->start, current_cluster->length());

                    std::string glyph_text = utf16_to_utf8(glyph_text_u16);
                    //                    std::cout << "Glyph text: " << glyph_text << std::endl;

                    // One glyph may have multiple codepoints.
                    // E.g. स् = स + ्
                    std::u32string glyph_text_u32;
                    utf8_to_utf32(glyph_text, glyph_text_u32);

                    Glyph glyph;

                    glyph.start = current_cluster->start;
                    glyph.end = current_cluster->end;

                    glyph.ascent = ascent;
                    glyph.descent = descent;

                    glyph.codepoints = glyph_text_u32;

                    glyph.text = glyph_text;

                    // Codepoint property is replaced with glyph ID after shaping.
                    glyph.index = info.codepoint;

                    glyph.script = run_script;

                    // Mark line breaks, so they're not drawn.
                    if (glyph_text == "\n") {
                        glyph.skip_drawing = true;
                    } else {
                        glyph.x_offset = (float)pos.x_offset * scale;
                        glyph.y_offset = (float)pos.y_offset * scale * -1.0;

                        glyph.x_advance = (float)pos.x_advance * scale;

                        // Debug
                        // {
                        //     int bitmap_width;
                        //     int bitmap_height;
                        //     int bitmap_xoffset;
                        //     int bitmap_yoffset;
                        //
                        //     auto bitmap_data = stbtt_GetGlyphBitmap(&stbtt_info,
                        //                                             scale,
                        //                                             scale,
                        //                                             glyph.index,
                        //                                             &bitmap_width,
                        //                                             &bitmap_height,
                        //                                             &bitmap_xoffset,
                        //                                             &bitmap_yoffset);
                        //
                        //     stbi_write_png(("glyph_bitmap_" + glyph.text + ".png").c_str(),
                        //                    bitmap_width,
                        //                    bitmap_height,
                        //                    1,
                        //                    bitmap_data,
                        //                    bitmap_width);
                        //
                        //     int _ = 0;
                        // }

                        para_width += glyph.x_advance;

                        font_to_use->populate_glyph_color_layers(glyph, scale);

                        // Get glyph path.
                        glyph.path = font_to_use->get_glyph_path(glyph.index, scale);

                        // The glyph's layout box in the glyph's local coordinates.
                        // The origin is the baseline. The Y axis is downward.
                        glyph.box = RectF(0, (float)-ascent, glyph.x_advance, (float)-descent);

                        // Get the glyph path's bounding box. The Y axis points down.
                        RectI bounding_box = font_to_use->get_glyph_bounds(glyph.index, scale);

                        // BBox in the glyph's local coordinates.
                        glyph.bbox = bounding_box.to_f32();
                    }

                    glyphs.push_back(glyph);
                }

                hb_buffer_destroy(hb_buffer);
            }

            // Record glyph start and end in the new paragraph.
            Line para{};
            para.glyph_ranges = {para_glyph_start, glyphs.size()};
            para.rtl = para_is_rtl;
            para.width = para_width;
            paragraphs.push_back(para);
        }
    } while (false);

    ubidi_close(line_bidi);
    ubidi_close(para_bidi);
}

#else

    #define FRIBIDI_MAX_STR_LEN 65000

void Font::get_glyphs(TextServer *text_server,
                      const std::string &text,
                      uint32_t font_size,
                      std::vector<Glyph> &glyphs,
                      std::vector<Line> &paragraphs) {
    glyphs.clear();
    paragraphs.clear();

    // uint32_t units_per_em = hb_face_get_upem(harfbuzz_data->face);

    std::u32string text_u32;
    utf8_to_utf32(text, text_u32);

    // Separation into paragraphs.
    std::vector<Pathfinder::Range> para_ranges_unicode;
    {
        int new_para_start_idx = 0;
        for (int char_idx = 0; char_idx < text_u32.size(); char_idx++) {
            if (text_u32[char_idx] == 10) {
                para_ranges_unicode.emplace_back((uint32_t)new_para_start_idx, (uint32_t)char_idx + 1);
                new_para_start_idx = char_idx + 1;
            }
        }

        if (!text_u32.empty() && text_u32.back() != 10) {
            para_ranges_unicode.emplace_back((uint32_t)new_para_start_idx, (uint32_t)text_u32.size());
        }
    }

    int para_count = para_ranges_unicode.size();

    // Go through paragraphs.
    for (int para_index = 0; para_index < para_count; para_index++) {
        // Paragraph start and end in the whole text. Unit: u32char.
        int para_start = para_ranges_unicode[para_index].start;
        int para_end = para_ranges_unicode[para_index].end;
        int para_length = para_end - para_start;

        auto para_text_u32 = text_u32.substr(para_start, para_length);
        auto para_text = utf32_to_utf8(para_text_u32);

        // Get FriBidiChar data.
        std::vector<FriBidiChar> fribidi_in_char(FRIBIDI_MAX_STR_LEN);
        const FriBidiStrIndex fribidi_len = fribidi_charset_to_unicode(
            FRIBIDI_CHAR_SET_UTF8, para_text.c_str(), para_text.size(), fribidi_in_char.data());

        assert(fribidi_len < FRIBIDI_MAX_STR_LEN && fribidi_len == para_text_u32.size());
        fribidi_in_char.resize(fribidi_len);

        std::vector<FriBidiChar> fribidi_visual_char(fribidi_len);
        std::vector<FriBidiLevel> embedding_level_list(fribidi_len);
        std::vector<FriBidiStrIndex> position_logical_to_visual_list(fribidi_len);
        std::vector<FriBidiStrIndex> position_visual_to_logical_list(fribidi_len);

        // See https://www.unicode.org/reports/tr9/#Bidirectional_Character_Types
        FriBidiCharType fribidi_pbase_dir = fribidi_get_bidi_type(fribidi_in_char.front());

        // Logical list to visual list.
        // This function only handles one-line paragraphs.
        const FriBidiLevel max_level = fribidi_log2vis(fribidi_in_char.data(),
                                                       fribidi_len,
                                                       &fribidi_pbase_dir,
                                                       fribidi_visual_char.data(),
                                                       position_logical_to_visual_list.data(),
                                                       position_visual_to_logical_list.data(),
                                                       embedding_level_list.data());
        assert(max_level != 0);

        // if (max_level) {
        //     std::string string_formatted_ptr(FRIBIDI_MAX_STR_LEN, 0);
        //     const FriBidiStrIndex new_len = fribidi_unicode_to_charset(
        //         FRIBIDI_CHAR_SET_UTF8, fribidi_visual_char.data(), fribidi_len, string_formatted_ptr.data());
        //     assert(new_len < FRIBIDI_MAX_STR_LEN);
        //     string_formatted_ptr.resize(new_len);
        // }

        // std::string para_text = utf32_to_utf8(text_u32.substr(para_start, para_end));
        // std::cout << "Paragraph text: " << para_text << std::endl;
        // std::cout << "Paragraph range: " << para_start << " -> " << para_end << std::endl;

        bool para_is_rtl = false;

        // The width of the paragraph in a single line.
        float para_width = 0;

        // The first glyph in the new paragraph.
        size_t para_glyph_start = glyphs.size();

        // Get run count in the current paragraph.

        std::vector<Pathfinder::Range> logical_para_runs;
        std::vector<signed char> logical_para_levels;
        {
            signed char current_level = embedding_level_list[0];
            logical_para_levels.push_back(current_level);

            int new_run_start_idx = 0;

            for (int char_idx = 0; char_idx < para_length; char_idx++) {
                signed char level = embedding_level_list[char_idx];
                if (level != current_level) {
                    logical_para_runs.push_back({(uint32_t)new_run_start_idx, (uint32_t)char_idx});
                    new_run_start_idx = char_idx;
                    current_level = level;

                    logical_para_levels.push_back(level);
                }
            }

            logical_para_runs.push_back({(uint32_t)new_run_start_idx, (uint32_t)para_end});
        }

        // Reorder runs from logical to visual.
        std::vector<Pathfinder::Range> para_runs;
        std::vector<signed char> para_levels;
        for (const auto &char_idx : position_visual_to_logical_list) {
            for (int run_idx = 0; run_idx < logical_para_runs.size(); run_idx++) {
                auto run = logical_para_runs[run_idx];
                auto level = logical_para_levels[run_idx];

                if (char_idx == run.start) {
                    para_runs.push_back(run);
                    para_levels.push_back(level);
                }
            }
        }

        int32_t run_count = para_levels.size();

        for (int32_t run_index = 0; run_index < run_count; run_index++) {
            signed char level = para_levels[run_index];

            bool run_is_rtl = level % 2 == 1;

            para_is_rtl |= run_is_rtl;
        }

        std::vector<Pathfinder::Range> para_clusters;

        // Go through runs.
        for (int32_t run_index = 0; run_index < run_count; run_index++) {
            signed char level = para_levels[run_index];
            auto run_range = para_runs[run_index];

            // Run start and end in the paragraph.
            int32_t run_start = run_range.start;
            int32_t run_length = run_range.end - run_range.start;

            bool run_is_rtl = level % 2 == 1;

            // Get run text from the whole text.
            std::u32string run_text_u32 = para_text_u32.substr(run_range.start, run_length);

            // Separate the run into script groups, so we can fall back font when necessary.
            auto run_script_ranges = get_text_script(run_text_u32);

            if (run_is_rtl) {
                std::reverse(run_script_ranges.begin(), run_script_ranges.end());
            }

            for (const auto &script_range : run_script_ranges) {
                auto script = script_range.first;
                auto script_range_in_run = script_range.second;

                uint32_t script_start = run_start + script_range_in_run.start;
                uint32_t script_end = run_start + script_range_in_run.end;
                uint32_t script_length = script_end - script_start;

                std::u32string script_text_u32 = para_text_u32.substr(script_start, script_length);

                Font *font_to_use = this;
                if (allow_fallback && !glyphs_exist_in_font(script_text_u32, this)) {
                    auto script_font = text_server->get_font_for_script(script);
                    if (!script_font) {
                        script_font = text_server->get_font_for_script(Script::Common);
                    }

                    if (script_font) {
                        font_to_use = script_font.get();
                    }
                }

                float ascent, descent;
                float scale = font_to_use->update_metrics(font_size, ascent, descent);

                // Buffers are sequences of Unicode characters that use the same font
                // and have the same text direction, script, and language.
                hb_buffer_t *hb_buffer = hb_buffer_create();

                // Item offset and length should represent a specific run.
                hb_buffer_add_utf32(hb_buffer,
                                    reinterpret_cast<const uint32_t *>(para_text_u32.c_str()),
                                    -1,
                                    script_start,
                                    script_length);

                hb_buffer_set_direction(hb_buffer, run_is_rtl ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
                hb_buffer_set_script(hb_buffer, to_harfbuzz_script(script));

                hb_shape(font_to_use->harfbuzz_data->font, hb_buffer, nullptr, 0);

                unsigned int glyph_count;
                hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(hb_buffer, &glyph_count);
                hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(hb_buffer, &glyph_count);

                std::vector<hb_glyph_info_t> debug_glyph_info(glyph_count);
                for (int i = 0; i < glyph_count; i++) {
                    debug_glyph_info[i] = glyph_info[i];
                }

                // Shaped glyph positions will always be in one line (regardless of line breaks).
                for (int i = 0; i < glyph_count; i++) {
                    auto &info = glyph_info[i];
                    auto &pos = glyph_pos[i];

                    // Cluster unit is u32char, so it should be worked with std::u32string instead of std::string.
                    std::optional<Pathfinder::Range> current_cluster;
                    if (!run_is_rtl) {
                        if (i < glyph_count - 1) {
                            // Multiple glyphs may share the same cluster.
                            for (int j = 1; i + j < glyph_count; j++) {
                                if (info.cluster != glyph_info[i + j].cluster) {
                                    current_cluster = {info.cluster, glyph_info[i + j].cluster};
                                    break;
                                }
                            }
                        }
                        if (!current_cluster.has_value()) {
                            current_cluster = {info.cluster, run_range.start + script_range_in_run.end};
                        }
                    } else {
                        if (i > 0) {
                            // Multiple glyphs may share the same cluster.
                            for (int j = 1; i - j >= 0; j++) {
                                if (info.cluster != glyph_info[i - j].cluster) {
                                    current_cluster = {info.cluster, glyph_info[i - j].cluster};
                                    break;
                                }
                            }
                        }
                        if (!current_cluster.has_value()) {
                            current_cluster = {info.cluster, run_range.start + script_range_in_run.end};
                        }
                    }

                    Pathfinder::Range absolute_cluster = {para_start + current_cluster->start,
                                                          para_start + current_cluster->end};
                    para_clusters.push_back(absolute_cluster);

                    std::u32string glyph_text_u32 =
                        para_text_u32.substr(current_cluster->start, current_cluster->length());

                    std::string glyph_text = utf32_to_utf8(glyph_text_u32);
                    //                    std::cout << "Glyph text: " << glyph_text << std::endl;

                    Glyph glyph;

                    glyph.start = para_start + current_cluster->start;
                    glyph.end = para_start + current_cluster->end;

                    glyph.ascent = ascent;
                    glyph.descent = descent;

                    // One glyph may have multiple codepoints.
                    // E.g. स् = स + ्
                    glyph.codepoints = glyph_text_u32;

                    glyph.text = glyph_text;

                    // Codepoint property is replaced with glyph ID after shaping.
                    glyph.index = info.codepoint;

                    glyph.script = script;

                    // Mark line breaks, so they're not drawn.
                    if (glyph_text == "\n") {
                        glyph.skip_drawing = true;
                    } else {
                        glyph.x_offset = (float)pos.x_offset * scale;
                        glyph.y_offset = (float)pos.y_offset * scale * -1.0;

                        glyph.x_advance = (float)pos.x_advance * scale;

                        // Debug
                        // {
                        //     int bitmap_width;
                        //     int bitmap_height;
                        //     int bitmap_xoffset;
                        //     int bitmap_yoffset;
                        //
                        //     auto bitmap_data = stbtt_GetGlyphBitmap(&stbtt_info,
                        //                                             scale,
                        //                                             scale,
                        //                                             glyph.index,
                        //                                             &bitmap_width,
                        //                                             &bitmap_height,
                        //                                             &bitmap_xoffset,
                        //                                             &bitmap_yoffset);
                        //
                        //     stbi_write_png(("glyph_bitmap_" + glyph.text + ".png").c_str(),
                        //                    bitmap_width,
                        //                    bitmap_height,
                        //                    1,
                        //                    bitmap_data,
                        //                    bitmap_width);
                        //
                        //     int _ = 0;
                        // }

                        para_width += glyph.x_advance;

                        font_to_use->populate_glyph_color_layers(glyph, scale);

                        // Get glyph path.
                        glyph.path = font_to_use->get_glyph_path(glyph.index, scale);

                        // The glyph's layout box in the glyph's local coordinates.
                        // The origin is the baseline. The Y axis is downward.
                        glyph.box = RectF(0, -ascent, glyph.x_advance, -descent);

                        // Get the glyph path's bounding box. The Y axis points down.
                        RectI bounding_box = font_to_use->get_glyph_bounds(glyph.index, scale);

                        // BBox in the glyph's local coordinates.
                        glyph.bbox = bounding_box.to_f32();
                    }

                    glyphs.push_back(glyph);
                }

                hb_buffer_destroy(hb_buffer);
            }
        }

        // Record glyph start and end in the new paragraph.
        Line para{};
        para.glyph_ranges = {para_glyph_start, glyphs.size()};
        para.rtl = para_is_rtl;
        para.width = para_width;
        para.clusters = para_clusters;
        paragraphs.push_back(para);
    }
}

#endif

void Font::get_glyphs(TextServer *text_server,
                      const std::vector<TextSpan> &spans,
                      uint32_t font_size,
                      std::vector<Glyph> &glyphs,
                      std::vector<Line> &paragraphs) {
    std::string full_text;
    for (const auto &span : spans) {
        full_text += span.text;
    }

    get_glyphs(text_server, full_text, font_size, glyphs, paragraphs);

    // Map glyphs back to spans.
    // We need to know the character offset of each span.
#ifndef VECGUI_USE_FRIBIDI
    // ICU version uses UTF-16 offsets.
    std::vector<size_t> span_offsets;
    size_t current_offset = 0;
    for (const auto &span : spans) {
        span_offsets.push_back(current_offset);
        std::u16string span_u16;
        utf8_to_utf16(span.text, span_u16);
        current_offset += span_u16.size();
    }
#else
    // FriBidi version uses UTF-32 offsets.
    std::vector<size_t> span_offsets;
    size_t current_offset = 0;
    for (const auto &span : spans) {
        span_offsets.push_back(current_offset);
        std::u32string span_u32;
        utf8_to_utf32(span.text, span_u32);
        current_offset += span_u32.size();
    }
#endif

    for (auto &glyph : glyphs) {
        for (int i = spans.size() - 1; i >= 0; --i) {
            if (glyph.start >= span_offsets[i]) {
                glyph.style = spans[i].style;

                // Local font size.
                if (spans[i].style.font_size != font_size) {
                    float scale = (float)spans[i].style.font_size / (float)font_size;

                    glyph.bbox = glyph.bbox * scale;
                    glyph.box = glyph.box * scale;
                    glyph.x_advance = glyph.x_advance * scale;
                    glyph.y_advance = glyph.y_advance * scale;
                    glyph.ascent = glyph.ascent * scale;
                    glyph.descent = glyph.descent * scale;

                    glyph.path.transform(Transform2::from_scale(Vec2F(scale)));
                }

                break;
            }
        }
    }
}

uint16_t Font::find_glyph_index_by_codepoint(int codepoint) {
    return stbtt_FindGlyphIndex(stbtt_info, codepoint);
}

RectI Font::get_glyph_bounds(uint16_t glyph_index, float scale) const {
    RectI bounding_box;

    stbtt_GetGlyphBitmapBox(stbtt_info,
                            glyph_index,
                            scale,
                            scale,
                            &bounding_box.left,
                            &bounding_box.top,
                            &bounding_box.right,
                            &bounding_box.bottom);

    if (glyph_index == 0 && bounding_box.left == bounding_box.right) {
        float ascent, descent, width;
        get_tofu_metrics(scale, ascent, descent, width);

        bounding_box.left = 0;
        bounding_box.top = (int)-ascent;
        bounding_box.right = (int)width;
        bounding_box.bottom = (int)-descent;
    }

    return bounding_box;
}

float Font::get_glyph_advance(uint16_t glyph_index, float scale) const {
    // The horizontal distance to increment (for left-to-right writing) or decrement (for right-to-left writing)
    // the pen position after a glyph has been rendered when processing text.
    // It is always positive for horizontal layouts, and zero for vertical ones.
    int advance_width;

    // The horizontal distance from the current pen position to the glyph's left bbox edge.
    // It is positive for horizontal layouts, and in most cases negative for vertical ones.
    int left_side_bearing;

    stbtt_GetGlyphHMetrics(stbtt_info, glyph_index, &advance_width, &left_side_bearing);

    float advance = (float)advance_width * scale;

    if (glyph_index == 0 && advance == 0) {
        float ascent, descent, width;
        get_tofu_metrics(scale, ascent, descent, width);
        advance = width;
    }

    return advance;
}

void Font::get_tofu_metrics(float scale, float &ascent, float &descent, float &width) const {
    int unscaled_ascent;
    int unscaled_descent;
    int unscaled_line_gap;
    stbtt_GetFontVMetrics(stbtt_info, &unscaled_ascent, &unscaled_descent, &unscaled_line_gap);

    ascent = (float)unscaled_ascent * scale;
    descent = (float)unscaled_descent * scale;
    width = ascent * 0.6f;
}

bool Font::is_valid() const {
    return !font_data.empty();
}

} // namespace vecgui
