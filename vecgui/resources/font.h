#pragma once

#include <pathfinder/prelude.h>

#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

#include "../common/context.h"
#include "../common/geometry.h"
#include "../common/utils.h"
#include "resource.h"

struct stbtt_fontinfo;

namespace vecgui {

class TextServer;

template <typename T>
void utf8_to_utf16(const std::string &source, std::basic_string<T> &result) {
    result.clear();
    for (size_t i = 0; i < source.size();) {
        uint32_t cp = 0;
        unsigned char c = (unsigned char)source[i];
        size_t len = 0;
        if (c <= 0x7F) {
            cp = c;
            len = 1;
        } else if ((c & 0xE0) == 0xC0) {
            cp = c & 0x1F;
            len = 2;
        } else if ((c & 0xF0) == 0xE0) {
            cp = c & 0x0F;
            len = 3;
        } else if ((c & 0xF8) == 0xF0) {
            cp = c & 0x07;
            len = 4;
        } else {
            // Skip invalid start byte
            i++;
            continue;
        }

        if (i + len > source.size()) break;

        bool invalid_seq = false;
        for (size_t j = 1; j < len; ++j) {
            c = (unsigned char)source[i + j];
            if ((c & 0xC0) != 0x80) {
                invalid_seq = true;
                break;
            }
            cp = (cp << 6) | (c & 0x3F);
        }

        if (invalid_seq) {
            i++; // Skip only the first byte of invalid sequence
            continue;
        }
        i += len;

        if (cp <= 0xFFFF) {
            result.push_back(static_cast<T>(cp));
        } else if (cp <= 0x10FFFF) {
            cp -= 0x10000;
            result.push_back(static_cast<T>((cp >> 10) + 0xD800));
            result.push_back(static_cast<T>((cp & 0x3FF) + 0xDC00));
        } else {
            // Skip invalid codepoint
        }
    }
}

template <typename T>
std::string utf16_to_utf8(const std::basic_string<T> &source) {
    std::string result;
    for (size_t i = 0; i < source.size(); ++i) {
        uint32_t cp = static_cast<uint32_t>(source[i]);
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < source.size()) {
            uint32_t low = static_cast<uint32_t>(source[i + 1]);
            if (low >= 0xDC00 && low <= 0xDFFF) {
                cp = ((cp - 0xD800) << 10) + (low - 0xDC00) + 0x10000;
                ++i;
            }
        }

        if (cp <= 0x7F) {
            result.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7FF) {
            result.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            result.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0x10FFFF) {
            result.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            // Skip invalid codepoint
        }
    }
    return result;
}

template <typename T>
void utf8_to_utf32(const std::string &source, std::basic_string<T> &result) {
    result.clear();
    for (size_t i = 0; i < source.size();) {
        uint32_t cp = 0;
        unsigned char c = (unsigned char)source[i];
        size_t len = 0;
        if (c <= 0x7F) {
            cp = c;
            len = 1;
        } else if ((c & 0xE0) == 0xC0) {
            cp = c & 0x1F;
            len = 2;
        } else if ((c & 0xF0) == 0xE0) {
            cp = c & 0x0F;
            len = 3;
        } else if ((c & 0xF8) == 0xF0) {
            cp = c & 0x07;
            len = 4;
        } else {
            // Skip invalid start byte
            i++;
            continue;
        }

        if (i + len > source.size()) break;

        bool invalid_seq = false;
        for (size_t j = 1; j < len; ++j) {
            c = (unsigned char)source[i + j];
            if ((c & 0xC0) != 0x80) {
                invalid_seq = true;
                break;
            }
            cp = (cp << 6) | (c & 0x3F);
        }

        if (invalid_seq) {
            i++; // Skip only the first byte of invalid sequence
            continue;
        }

        if (cp <= 0x10FFFF) {
            result.push_back(static_cast<T>(cp));
        } else {
            // Skip invalid codepoint
        }
        i += len;
    }
}

template <typename T>
std::string utf32_to_utf8(const std::basic_string<T> &source) {
    std::string result;
    for (auto cp_t : source) {
        uint32_t cp = static_cast<uint32_t>(cp_t);
        if (cp <= 0x7F) {
            result.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7FF) {
            result.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            result.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0x10FFFF) {
            result.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            // Skip invalid codepoint
        }
    }
    return result;
}

// This should convert to whatever the system-wide character encoding
// is for the platform (UTF-32/Linux - UCS-2/Windows)
inline std::string ws_to_utf8(std::wstring const &s) {
    if constexpr (sizeof(wchar_t) == 2) {
        return utf16_to_utf8(s);
    } else {
        return utf32_to_utf8(s);
    }
}

inline std::wstring utf8_to_ws(std::string const &utf8) {
    std::wstring result;
    if constexpr (sizeof(wchar_t) == 2) {
        utf8_to_utf16(utf8, result);
    } else {
        utf8_to_utf32(utf8, result);
    }
    return result;
}

enum class GradientMappingMode {
    Span, // Gradient mapped to the entire span/batch bounds
    Glyph // Gradient mapped to each individual glyph's bounds
};

struct TextStyle {
    TextStyle() = default;
    TextStyle(ColorU color) : fill(color) {
    }

    uint32_t font_size = 24u;

    bool italic = false;
    bool bold = false;

    // Fill.
    std::variant<ColorU, Pathfinder::Gradient> fill = ColorU::white();

    // Stroke.
    ColorU stroke_color = ColorU::transparent_black();
    float stroke_width = 0;

    // Shadow.
    ColorU shadow_color = ColorU::transparent_black();
    float shadow_radius = 0;
    float shadow_strength = 1.0f;
    Vec2F shadow_offset;

    // Background.
    ColorU background_color = ColorU::transparent_black();
    float background_corner_radius = 0;
    float background_expand = 0;

    // Karaoke.
    float karaoke_progress = -1.0f; // < 0 means disabled. 0.0 to 1.0.
    ColorU karaoke_reached_color;

    // Clipping.
    float clipping_progress = -1.0f; // < 0 means disabled. 0.0 to 1.0.

    Transform2 local_transform;

    // Overall opacity.
    float opacity = 1.0f;

    GradientMappingMode gradient_mapping_mode = GradientMappingMode::Span;

    bool debug = false;

    ColorU get_fill_color() const {
        if (std::holds_alternative<ColorU>(fill)) {
            return std::get<ColorU>(fill);
        }
        return ColorU::white();
    }

    void set_fill_color(ColorU color) {
        fill = color;
    }

    bool operator==(const TextStyle &rhs) const {
        return fill == rhs.fill && font_size == rhs.font_size && italic == rhs.italic && bold == rhs.bold &&
               stroke_color == rhs.stroke_color && stroke_width == rhs.stroke_width &&
               shadow_color == rhs.shadow_color && shadow_radius == rhs.shadow_radius &&
               shadow_offset == rhs.shadow_offset && background_color == rhs.background_color &&
               background_corner_radius == rhs.background_corner_radius &&
               background_expand == rhs.background_expand && karaoke_progress == rhs.karaoke_progress &&
               karaoke_reached_color == rhs.karaoke_reached_color && clipping_progress == rhs.clipping_progress &&
               local_transform == rhs.local_transform && opacity == rhs.opacity &&
               gradient_mapping_mode == rhs.gradient_mapping_mode && debug == rhs.debug;
    }
};

struct TextSpan {
    std::string text;
    TextStyle style;
};

enum class Script {
    Common = 0,
    Arabic,
    Armenian,
    Bengali,
    Devanagari,
    Hebrew,
    Han,
    Hangul,
    Hiragana,
    Katakana,
    Thai,
};

struct GlyphLayer {
    uint16_t index = 0;
    std::variant<ColorU, Pathfinder::Gradient> fill = ColorU();
    Pathfinder::Path2d path;
};

// Text-context-dependent glyph data.
struct Glyph {
    // Glyph index (font specific). Zero for invalid glyphs.
    // A particular glyph ID within the font does not necessarily correlate to a predictable Unicode codepoint.
    uint16_t index = 0;

    std::u32string codepoints;

    std::string text;

    // Start codepoint index in the text.
    int start = 0;

    // End codepoint index in the text.
    int end = 0;

    bool emoji = false;

    std::vector<GlyphLayer> layers;

    Script script = Script::Common;

    float x_offset = 0; // Offset from the origin of the glyph on baseline.
    float y_offset = 0;

    float x_advance = 0; // Advance to the next glyph along baseline (x for horizontal layout, y for vertical).
    float y_advance = 0;

    float ascent = 0;
    float descent = 0;

    bool skip_drawing = false;

    // Glyph path. The points are in the glyph's baseline coordinates.
    Pathfinder::Path2d path;

    /// Glyph box in the baseline coordinates, which has nothing to do with the glyph position in the text paragraph.
    RectF box;

    /// Glyph path's bounding box in the baseline coordinates, which has nothing to do with the glyph position in the
    /// text paragraph.
    RectF bbox;

    TextStyle style; // 让每个 Glyph 携带样式
};

struct Line {
    Pathfinder::Range glyph_ranges;
    bool rtl = false;
    float width = 0;
    std::vector<Pathfinder::Range> clusters;
};

struct HarfBuzzData;

// A font is pointsize-carefree.
class Font {
public:
    Font() = default;

    static std::shared_ptr<Font> from_file(const GuiContext *context, const std::string &path);

    static std::shared_ptr<Font> from_memory(const std::vector<char> &bytes);

    ~Font();

    bool is_valid() const;

    Pathfinder::Path2d get_glyph_path(uint16_t glyph_index, float scale) const;

    /// Paragraphs and lines are different concepts.
    /// Paragraphs are seperated by line breaks, while lines are produced by further layouting.
    /// A paragraph may contain one or more lines.
    void get_glyphs(TextServer *text_server,
                    const std::vector<TextSpan> &spans,
                    uint32_t font_size,
                    std::vector<Glyph> &glyphs,
                    std::vector<Line> &paragraphs);

    void get_glyphs(TextServer *text_server,
                    const std::string &text,
                    uint32_t font_size,
                    std::vector<Glyph> &glyphs,
                    std::vector<Line> &paragraphs);

    uint16_t find_glyph_index_by_codepoint(int codepoint);

    float get_glyph_advance(uint16_t glyph_index, float scale) const;

    RectI get_glyph_bounds(uint16_t glyph_index, float scale) const;

    void populate_glyph_color_layers(Glyph &glyph, float scale) const;

    float update_metrics(uint32_t size, float &ascent, float &descent);

    std::shared_ptr<HarfBuzzData> harfbuzz_data;

private:
    /// Stores font data, should not be freed until font is deleted.
    unsigned char *stbtt_buffer{};

    stbtt_fontinfo *stbtt_info{};

    /// Will fall back to the default font for unfound glyphs.
    bool allow_fallback = true;

    // Obsoleted, caching glyphs requires setting a font size, which we don't do to a font.
    std::unordered_map<uint16_t, Glyph> glyph_cache;

    // Raw font data, read directly from a file or from memory.
    std::vector<char> font_data;

    void get_tofu_metrics(float scale, float &ascent, float &descent, float &width) const;
};

} // namespace vecgui
