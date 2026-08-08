#pragma once

#include <pathfinder/prelude.h>

#include <memory>

#include "../common/geometry.h"
#include "image.h"

namespace vecgui {

// TODO: Merge this with VectorTexture.
/// A thin wrapper over Pathfinder::Path2d.
struct VectorPath {
    Pathfinder::Path2d path2d;
    ColorU fill_color = ColorU();
    ColorU stroke_color = ColorU();
    float stroke_width = 0;
    float opacity = 1;
};

/// A SVG analogy.
class VectorImage : public Image {
public:
    explicit VectorImage(Vec2I _size);

    /// Create from an SVG file.
    VectorImage(const std::string &path, bool override_with_accent_color = false);

    /// Create empty with a specific size.
    static std::shared_ptr<VectorImage> from_empty(Vec2I _size);

    void add_path(const VectorPath &new_path);

    std::vector<VectorPath> &get_paths();

    std::shared_ptr<Pathfinder::SvgScene> get_svg_scene();

protected:
    std::vector<VectorPath> paths;

    std::shared_ptr<Pathfinder::SvgScene> svg_scene;
};

} // namespace vecgui
