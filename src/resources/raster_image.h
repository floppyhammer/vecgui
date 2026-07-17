#pragma once

#include <memory>

#include "image.h"

namespace vecgui {

class RasterImage final : public Image {
public:
    RasterImage(Vec2I size_);

    explicit RasterImage(const std::string &path);

    std::shared_ptr<Pathfinder::Image> image_data;
};

} // namespace vecgui
