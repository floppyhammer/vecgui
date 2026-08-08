#include "image.h"

namespace vecgui {

Vec2I Image::get_size() {
    return size;
}

ImageType Image::get_type() {
    return type;
}

} // namespace vecgui
