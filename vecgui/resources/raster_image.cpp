#include "raster_image.h"

#include "../common/utils.h"

// Already defined in Pathfinder.
// #define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <cstring>
#include <stdexcept>

namespace vecgui {

RasterImage::RasterImage(Vec2I size_) {
    type = ImageType::Raster;
    size = size_;
}

RasterImage::RasterImage(const std::string &path) {
    type = ImageType::Raster;

    // The STBI_rgb_alpha value forces the image to be loaded with an alpha channel,
    // even if it doesn't have one, which is nice for consistency with other textures in the future.
    int tex_width, tex_height, tex_channels;
    stbi_uc *pixels = stbi_load(path.c_str(), &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

    if (!pixels) {
        Logger::warn("Failed to load image file " + path, "vecgui");
        throw std::runtime_error("Failed to load texture image!");
    }

    size = {tex_width, tex_height};

    std::vector<ColorU> bytes(tex_width * tex_height * 4);
    memcpy(bytes.data(), pixels, bytes.size());
    image_data = std::make_shared<Pathfinder::Image>(size, bytes);

    // Clean up the original pixel array.
    stbi_image_free(pixels);
}

} // namespace vecgui
