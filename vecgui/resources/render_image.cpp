#include "render_image.h"

namespace vecgui {

RenderImage::RenderImage(Vec2I _size) {
    size = _size;

    const Pathfinder::TextureDescriptor desc = {
        size,
        Pathfinder::TextureFormat::Rgba8Unorm,
    };

    type = ImageType::Render;

    texture_ = RenderContext::get_singleton()->get_device()->create_texture(desc, "render image");
}

RenderImage::RenderImage(const std::shared_ptr<Pathfinder::Texture>& existing_texture) {
    size = existing_texture->get_size();
    type = ImageType::Render;
    texture_ = existing_texture;
}

} // namespace vecgui
