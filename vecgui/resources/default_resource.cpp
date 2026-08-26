#include "default_resource.h"

#include "font.h"
#include "opensans_regular_ttf.h"
#include "vecgui/servers/text_server.h"

namespace vecgui {

void DefaultResource::init(GuiContext* context, const bool dark_mode) {
    default_theme = dark_mode ? Theme::default_dark() : Theme::default_light();

    default_font = Font::from_memory(std::vector<char>(std::begin(DEFAULT_FONT_DATA), std::end(DEFAULT_FONT_DATA)));
    assert(default_font);

    auto text_server = context->text_server;
    auto ns = Font::from_file(context, get_asset_dir("fonts/NotoSans-Regular.ttf"));
    text_server->register_fallback_font(Script::Common, ns);
    text_server->register_fallback_font(Script::Arabic,
                                        Font::from_file(context, get_asset_dir("fonts/NotoSansArabic-Regular.ttf")));
    text_server->register_fallback_font(Script::Hangul,
                                        Font::from_file(context, get_asset_dir("fonts/NotoSansKR-Regular.ttf")));
    text_server->register_fallback_font(Script::Han,
                                        Font::from_file(context, get_asset_dir("fonts/NotoSansSC-Regular.ttf")));
    auto jp = Font::from_file(context, get_asset_dir("fonts/NotoSansJP-Regular.ttf"));
    text_server->register_fallback_font(Script::Hiragana, jp);
    text_server->register_fallback_font(Script::Katakana, jp);
    text_server->register_fallback_font(Script::Bengali,
                                        Font::from_file(context, get_asset_dir("fonts/NotoSansBengali-Regular.ttf")));
    text_server->register_fallback_font(Script::Thai,
                                        Font::from_file(context, get_asset_dir("fonts/NotoSansThai-Regular.ttf")));

    // Emoji.
    // text_server->register_emoji_font(Font::from_file(context, get_asset_dir("fonts/NotoColorEmoji-Regular.ttf")));
}

} // namespace vecgui
