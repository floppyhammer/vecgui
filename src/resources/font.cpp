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

#include <hb.h>

#include <gzip/decompress.hpp>
#include <gzip/utils.hpp>
#include <optional>

#include "../servers/engine.h"
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
        } else if (codepoint >= 0x4E00 && codepoint <= 0x9FFF) {
            scripts.push_back(Script::Cjk);
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

    HarfBuzzData() = default;

    explicit HarfBuzzData(const std::vector<char> &bytes) {
        // We need to keep bytes.data() valid for HarfBuzz to work properly.
        blob = hb_blob_create(bytes.data(), bytes.size(), HB_MEMORY_MODE_READONLY, nullptr, nullptr);
        face = hb_face_create(blob, 0);
        font = hb_font_create(face);
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

std::shared_ptr<Font> Font::from_file(const std::string &path) {
#ifndef __ANDROID__
    auto bytes = Pathfinder::load_file_as_bytes(path);
#else
    auto bytes = Pathfinder::load_asset(Engine::get_singleton()->asset_manager, path);
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

std::string Font::get_glyph_svg(uint16_t glyph_index) const {
    const char *data{};
    size_t data_size = stbtt_GetGlyphSVG(stbtt_info, glyph_index, &data);
    if (data_size > 0) {
        // // Check if compressed. Can check both gzip and zlib.
        // bool compressed = gzip::is_compressed(data, data_size);
        //
        // if (compressed) {
        //     std::string decompressed_data = gzip::decompress(data, data_size);
        //     return decompressed_data;
        // }
        //
        // return {data, data + data_size};
    }
    return {};
}

Pathfinder::Path2d Font::get_glyph_path(uint16_t glyph_index, float scale) const {
    Pathfinder::Path2d path;

    stbtt_vertex *vertices{};
    int num_vertices = stbtt_GetGlyphShape(stbtt_info, glyph_index, &vertices);

    // Glyph has no shape (e.g. Space).
    if (vertices == nullptr) {
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

void Font::get_glyphs(const std::string &text,
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

                float ascent, descent;
                float scale = update_metrics(font_size, ascent, descent);

                // Buffers are sequences of Unicode characters that use the same font
                // and have the same text direction, script, and language.
                hb_buffer_t *hb_buffer = hb_buffer_create();

                // Item offset and length should represent a specific run.
                hb_buffer_add_utf16(
                    hb_buffer, reinterpret_cast<const uint16_t *>(uchar_data), -1, para_start + logical_start, length);

                hb_buffer_set_direction(hb_buffer, run_is_rtl ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
                hb_buffer_set_script(hb_buffer, to_harfbuzz_script(run_script));

                hb_shape(harfbuzz_data->font, hb_buffer, nullptr, 0);

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

                        // Get glyph path.
                        glyph.path = get_glyph_path(glyph.index, scale);

                        // The glyph's layout box in the glyph's local coordinates.
                        // The origin is the baseline. The Y axis is downward.
                        glyph.box = RectF(0, (float)-ascent, glyph.x_advance, (float)-descent);

                        // Get the glyph path's bounding box. The Y axis points down.
                        RectI bounding_box = get_glyph_bounds(glyph.index, scale);

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

void Font::get_glyphs(const std::string &text,
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
                bool use_fallback_font = !glyphs_exist_in_font(script_text_u32, this);

                Font *font_to_use;
                if (allow_fallback && use_fallback_font) {
                    font_to_use = DefaultResource::get_singleton()->get_default_font().get();
                } else {
                    font_to_use = this;
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

void Font::get_glyphs(const std::vector<TextSpan> &spans,
                      uint32_t font_size,
                      std::vector<Glyph> &glyphs,
                      std::vector<Line> &paragraphs) {
    std::string full_text;
    for (const auto &span : spans) {
        full_text += span.text;
    }

    get_glyphs(full_text, font_size, glyphs, paragraphs);

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
                if (spans[i].style.font_size.has_value()) {
                    float scale = (float)spans[i].style.font_size.value() / (float)font_size;

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

    return (float)advance_width * scale;
}

bool Font::is_valid() const {
    return !font_data.empty();
}

} // namespace vecgui
