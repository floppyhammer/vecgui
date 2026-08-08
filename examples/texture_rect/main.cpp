#include "app.h"

using namespace vecgui;

using Pathfinder::Vec2;
using Pathfinder::Vec3;

class MyNode : public Node {
    void on_ready() override {
        auto raste_image = std::make_shared<RasterImage>(get_asset_dir("duck.png"));
        auto vector_image = std::make_shared<VectorImage>(get_asset_dir("icons/Node_Button.svg"));

        for (int i = 0; i < 6; i++) {
            auto texture_rect_svg = std::make_shared<TextureRect>();
            texture_rect_svg->set_position({400.0f, i * 100.0f});
            texture_rect_svg->set_size({200, 100});
            texture_rect_svg->set_texture(raste_image);
            add_child(texture_rect_svg);

            auto texture_rect_image = std::make_shared<TextureRect>();
            texture_rect_image->set_texture(vector_image);
            texture_rect_image->set_position({0.0f, i * 100.0f});
            texture_rect_image->set_size({200, 100});
            add_child(texture_rect_image);

            if (i == 0) {
                texture_rect_svg->set_stretch_mode(TextureRect::StretchMode::Keep);
                texture_rect_image->set_stretch_mode(TextureRect::StretchMode::Keep);
            }
            if (i == 1) {
                texture_rect_svg->set_stretch_mode(TextureRect::StretchMode::KeepCentered);
                texture_rect_image->set_stretch_mode(TextureRect::StretchMode::KeepCentered);
            }
            if (i == 2) {
                texture_rect_svg->set_stretch_mode(TextureRect::StretchMode::Scale);
                texture_rect_image->set_stretch_mode(TextureRect::StretchMode::Scale);
            }
            if (i == 3) {
                texture_rect_svg->set_stretch_mode(TextureRect::StretchMode::KeepAspect);
                texture_rect_image->set_stretch_mode(TextureRect::StretchMode::KeepAspect);
            }
            if (i == 4) {
                texture_rect_svg->set_stretch_mode(TextureRect::StretchMode::KeepAspectCentered);
                texture_rect_image->set_stretch_mode(TextureRect::StretchMode::KeepAspectCentered);
            }
            if (i == 5) {
                texture_rect_svg->set_stretch_mode(TextureRect::StretchMode::KeepCovered);
                texture_rect_image->set_stretch_mode(TextureRect::StretchMode::KeepCovered);
            }
        }
    }
};

int main() {
    App app({720, 720});

    app.get_tree_root()->add_child(std::make_shared<MyNode>());

    app.main_loop();

    return EXIT_SUCCESS;
}
