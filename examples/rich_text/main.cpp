#include <resources/default_resource.h>
#include "app.h"

using namespace vecgui;

class RichTextNode : public Node {
    void custom_ready() override {
        auto margin_container = std::make_shared<MarginContainer>();
        margin_container->set_margin_all(32);
        margin_container->set_anchor_flag(AnchorFlag::FullRect);
        add_child(margin_container);

        auto center_container = std::make_shared<Container>();
        margin_container->add_child(center_container);

        auto label = std::make_shared<Label>();
        label->set_font_size(48);

        // Clear default text and add spans with different styles.
        label->clear_spans();

        // 1. Bold Red
        TextStyle style1;
        style1.color = ColorU::red();
        style1.bold = true;
        label->add_span({"Rich ", style1});

        // 2. Italic Green
        TextStyle style2;
        style2.color = ColorU::green();
        style2.italic = true;
        label->add_span({"Text ", style2});

        // 3. Normal Blue
        TextStyle style3;
        style3.color = ColorU::blue();
        style3.shadow_color = ColorU::black();
        style3.shadow_radius = 16.0;
        style3.shadow_offset = Vec2F(8.0, 8.0);
        label->add_span({"Label\n", style3});

        // 4. Mixed Style in Chinese
        TextStyle style_cn;
        style_cn.color = ColorU::white();
        label->add_span({"你好，", style_cn});

        style_cn.color = ColorU::yellow();
        style_cn.bold = true;
        style_cn.background_color = ColorU::black();
        style_cn.background_corner_radius = 8.0;
        style_cn.background_padding = 2.0;
        label->add_span({"世界！", style_cn});

        // 5. Stroke style
        TextStyle style_stroke;
        style_stroke.color = ColorU::red();
        style_stroke.stroke_color = ColorU::black();
        style_stroke.stroke_width = 8.0f;
        label->add_span({"\nStroke Style", style_stroke});

        center_container->add_child(label);
    }
};

int main() {
    App app({1280, 720}, true);

    app.get_tree_root()->add_child(std::make_shared<RichTextNode>());

    app.main_loop();

    return EXIT_SUCCESS;
}
