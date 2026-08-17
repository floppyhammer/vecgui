#include "texture_rect.h"

#include <memory>

#include "../../common/context.h"
#include "../../common/utils.h"
#include "../../resources/default_resource.h"
#include "../../resources/render_image.h"
#include "../../servers/engine.h"
#include "../../servers/vector_server.h"

namespace vecgui {

TextureRect::TextureRect() {
    type = NodeType::TextureRect;
}

void TextureRect::set_texture(const std::shared_ptr<Image> &new_texture) {
    if (texture == new_texture) {
        return;
    }

    // Texture can be null.
    texture = new_texture;
    queue_relayout();
}

std::shared_ptr<Image> TextureRect::get_texture() const {
    return texture;
}

void TextureRect::update(double dt) {
    NodeUi::update(dt);
}

void TextureRect::draw() {
    NodeUi::draw();

    if (!visible_) {
        return;
    }

    if (texture) {
        auto global_position = get_global_position();

        auto texture_size = texture->get_size().to_f32();

        Transform2 transform;

        switch (stretch_mode) {
            case StretchMode::KeepCentered: {
                auto offset = (size - texture_size) * 0.5f;

                transform = Transform2::from_translation(global_position + offset);
            } break;
            case StretchMode::Scale: {
                if (texture_size.area() == 0) {
                    Logger::error("Vector texture size is invalid!", "vecgui");
                    return;
                }
                auto scale = size / texture_size;

                transform = Transform2::from_scale(scale).translate(global_position);
            } break;
            case StretchMode::KeepAspect:
            case StretchMode::KeepAspectCentered: {
                if (texture_size.area() == 0) {
                    Logger::error("Vector texture size is invalid!", "vecgui");
                    return;
                }

                auto tex_aspect_ratio = texture_size.x / texture_size.y;
                auto rect_aspect_ratio = size.x / size.y;

                Vec2F scale;
                if (tex_aspect_ratio > rect_aspect_ratio) {
                    scale = Vec2F(size.x / texture_size.x);
                } else {
                    scale = Vec2F(size.y / texture_size.y);
                }

                Vec2F effective_size = texture_size * scale;

                Vec2F offset;
                if (stretch_mode == StretchMode::KeepAspectCentered) {
                    offset = (size - effective_size) * 0.5f;
                }

                transform = Transform2::from_scale(scale).translate(global_position + offset);
            } break;
            default: {
                transform = Transform2::from_translation(global_position);
            } break;
        }

        auto context = get_context();
        if (!context) {
            return;
        }

        auto vector_server = context->vector_server;

        if (texture->get_type() == ImageType::Raster) {
            auto raster_image = static_cast<RasterImage *>(texture.get());
            vector_server->draw_raster_image(*raster_image, transform);
        } else if (texture->get_type() == ImageType::Vector) {
            auto vector_image = static_cast<VectorImage *>(texture.get());
            vector_server->draw_vector_image(*vector_image, transform);
        } else if (texture->get_type() == ImageType::Render) {
            auto render_image = static_cast<RenderImage *>(texture.get());
            vector_server->draw_render_image(*render_image, transform);
        } else {
            Logger::error("Unsupported texture type!", "vecgui");
        }
    }
}

void TextureRect::calc_minimum_size() {
    calculated_minimum_size = Vec2F(0);
    if (texture) {
        if (StretchMode::KeepCentered == stretch_mode || StretchMode::Keep == stretch_mode) {
            calculated_minimum_size = texture->get_size().to_f32();
        }
    }
}

void TextureRect::set_stretch_mode(StretchMode new_mode) {
    if (stretch_mode == new_mode) {
        return;
    }
    stretch_mode = new_mode;
    queue_relayout();
}

} // namespace vecgui
