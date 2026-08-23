#include "vecgui/app.h"
#include "vecgui/resources/default_resource.h"

using namespace vecgui;

class RichTextNode : public Node {
    void on_ready() override {
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
        style1.set_fill_color(ColorU::red());
        style1.bold = true;
        label->add_span({"Rich ", style1});

        // 2. Italic Green
        TextStyle style2;
        style2.set_fill_color(ColorU::green());
        style2.italic = true;
        style2.font_size = 64;
        label->add_span({"Text", style2});

        // 3. Normal Blue
        TextStyle style3;
        style3.set_fill_color(ColorU::blue());
        style3.font_size = 12;
        style3.shadow_color = ColorU::black();
        style3.shadow_strength = 1.0;
        style3.shadow_radius = 8.0;
        style3.shadow_offset = Vec2F(4.0, 4.0);
        label->add_span({" Label\n", style3});

        // 4. Mixed Style in Chinese
        TextStyle style_cn;
        style_cn.set_fill_color(ColorU::white());
        label->add_span({"你好，", style_cn});

        style_cn.set_fill_color(ColorU::yellow());
        style_cn.background_color = ColorU::black();
        style_cn.background_corner_radius = 8.0;
        style_cn.background_padding = 2.0;
        style_cn.font_size = 128;
        label->add_span({"世界！", style_cn});

        // 5. Stroke style
        TextStyle style_stroke;
        style_stroke.set_fill_color(ColorU::red());
        style_stroke.stroke_color = ColorU::black();
        style_stroke.stroke_width = 6.0f;
        label->add_span({"\nStroke Style", style_stroke});

        // 6. Gradient Style
        TextStyle style_grad;
        // Now using normalized coordinates: (0,0) to (1,0)
        // (0,0) is top-left of the text span, (1,0) is top-right.
        Pathfinder::Gradient grad = Pathfinder::Gradient::linear(Pathfinder::LineSegmentF({0, 0}, {1, 0}));
        grad.add_color_stop(ColorU::red(), 0.0f);
        grad.add_color_stop(ColorU::yellow(), 0.5f);
        grad.add_color_stop(ColorU::blue(), 1.0f);
        style_grad.fill = grad;
        style_grad.font_size = 64;
        style_grad.bold = true;
        style_grad.stroke_color = ColorU::white();
        style_grad.stroke_width = 2.0f;
        style_grad.gradient_mapping_mode = GradientMappingMode::Span;
        label->add_span({"\nGradient Text", style_grad});

        center_container->add_child(label);
    }
};

int main() {
    App app({1280, 720});
    app.get_tree_root()->add_child(std::make_shared<RichTextNode>());

    app.main_loop();

    return EXIT_SUCCESS;
}
