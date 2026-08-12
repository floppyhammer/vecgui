#include <vecgui/app.h>
#include <vecgui/resources/default_resource.h>

#include <iostream>
#include <random>

using namespace vecgui;

using Pathfinder::Vec2;
using Pathfinder::Vec3;

class MyNode : public Node {
    void on_ready() override {
        auto margin_container = std::make_shared<MarginContainer>();
        margin_container->set_margin_all(32);
        margin_container->set_anchor_flag(AnchorFlag::FullRect);
        add_child(margin_container);

        auto font = Font::from_file("assets/unifont-17.0.03.otf");

        auto s_container = std::make_shared<SplitContainer>();
        s_container->set_separation(16);
        margin_container->add_child(s_container);

        std::string text = "";
        // text += "👍😁😂\n";                      // Emoji
        text += "Hello world!\n";                  // English
        text += "你好世界！\n";                  // Chinese
        text += "こんにちは世界！\n";            // Japanese
        text += "안녕 세계\n";                   // Korean
        text += "مرحبا بالعالم!\n";              // Arabic
        text += "ওহে বিশ্ব!\n";                   // Bengali
        text += "สวัสดีชาวโลก!\n";                 // Thai
        text += "سلام دنیا!\n";                  // Persian
        text += "नमस्ते दुनिया!\n";                 // Hindi
        text += "Chào thế giới!\n";              // Vietnamese
        text += "Привет, мир\n";                 // Russian
        text += "שלום עולם!\n\n";                // Hebrew
        text += "Hello123!مرحبا٠١٢!你好123！\n"; // Mixed languages

        // No word wrapping.
        {
            auto label = std::make_shared<Label>();
            label->set_text(text);
            label->container_sizing.flag_h = ContainerSizingFlag::Fill;
            label->set_font(font);
            label->set_font_size(32);
            label->debug_box = StyleBox::simple_outline();

            s_container->add_child(label);
        }

        // Word wrapping.
        {
            auto label = std::make_shared<Label>();
            label->set_word_wrap(true);
            label->set_text(text);
            label->container_sizing.flag_h = ContainerSizingFlag::Fill;
            label->set_font(font);
            label->debug_box = StyleBox::simple_outline();

            s_container->add_child(label);
        }
    }
};

int main() {
    App app({1280, 720});

    app.get_tree_root()->add_child(std::make_shared<MyNode>());

    app.main_loop();

    return EXIT_SUCCESS;
}
