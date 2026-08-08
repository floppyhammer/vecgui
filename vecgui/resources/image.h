#pragma once

#include "../common/geometry.h"

namespace vecgui {

enum class ImageType {
    Raster,
    Vector,
    Render,
    Max,
};

class Image {
public:
    Image() = default;

    Vec2I get_size();

    ImageType get_type();

protected:
    ImageType type = ImageType::Max;

    Vec2I size;
};

} // namespace vecgui
