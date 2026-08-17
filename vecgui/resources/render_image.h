#pragma once

#include <pathfinder/prelude.h>

#include "../common/context.h"
#include "../common/geometry.h"
#include "../servers/render_server.h"
#include "image.h"

namespace vecgui {

class RenderImage : public Image {
public:
    explicit RenderImage(const GuiContext* context, Vec2I _size);

    explicit RenderImage(const std::shared_ptr<Pathfinder::Texture>& existing_texture);

    std::shared_ptr<Pathfinder::Texture> get_texture() const {
        return texture_;
    }

protected:
    std::shared_ptr<Pathfinder::Texture> texture_;
};

} // namespace vecgui
