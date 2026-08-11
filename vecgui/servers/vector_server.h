#pragma once

#include <pathfinder/prelude.h>

#include "../common/geometry.h"
#include "../resources/font.h"
#include "../resources/raster_image.h"
#include "../resources/render_image.h"
#include "../resources/style_box.h"
#include "../resources/vector_image.h"

namespace vecgui {

/// All visible shapes will be collected by the vector server and drawn at once.
class VectorServer final {
public:
    static VectorServer *get_singleton() {
        static VectorServer singleton;
        return &singleton;
    }

    VectorServer() = default;

    void init(Pathfinder::Vec2I size,
              const std::shared_ptr<Pathfinder::Device> &device,
              const std::shared_ptr<Pathfinder::Queue> &queue,
              Pathfinder::RenderMode mode);

    void set_dst_texture(const std::shared_ptr<Pathfinder::Texture> &texture);

    void cleanup();

    void set_canvas_size(Vec2I new_size);

    void submit_and_clear();

    void draw_line(Vec2F start, Vec2F end, float width, ColorU color);

    void draw_rectangle(const RectF &rect, float line_width, ColorU color, bool fill);

    void draw_circle(Vec2F center, float radius, float line_width, bool fill, ColorU color);

    void draw_path(VectorPath &vector_path, Transform2 transform);

    void draw_raster_image(const RasterImage &image, const Transform2 &transform);

    void draw_vector_image(VectorImage &image, Transform2 transform);

    void draw_render_image(RenderImage &image, Transform2 transform);

    void draw_style_box(const StyleBox &style_box, Vec2F position, Vec2F size, float alpha = 1.0f);

    void draw_style_box(const StyleBox &style_box, const Transform2 &transform, Vec2F size, float alpha = 1.0f);

    void draw_style_line(const StyleLine &style_line, const Vec2F &start, const Vec2F &end);

    void draw_style_line(const StyleLine &style_line,
                         const Transform2 &transform,
                         const Vec2F &start,
                         const Vec2F &end);

    /**
     * @param transform
     * @param clip_box Enable content clip, portion of anything drawn afterward
     * outside the clip box will not show. The rect is in local coordinates, and the transform will be applied to it.
     * We shouldn't use a clip path to achieve general content clip (like scrolling)
     * since it's quite performance heavy and easily produces nested clipping.
     */
    void draw_glyphs(std::vector<Glyph> &glyphs,
                     std::vector<Vec2F> &glyph_positions,
                     TextStyle text_style,
                     const Transform2 &transform,
                     const RectF &clip_box,
                     float alpha = 1.0f,
                     const std::vector<Line> &lines = {});

    std::shared_ptr<Pathfinder::SvgScene> load_svg(const std::string &path, bool override_with_accent_color = false);

    std::shared_ptr<Pathfinder::Canvas> get_canvas() const;

    float get_global_scale() const;

    void set_global_scale(float new_scale);

    // Only used with ScrollContainer.
    Transform2 global_transform_offset{};

private:
    // Never expose this.
    std::shared_ptr<Pathfinder::Canvas> canvas;

    float global_scale_ = 1.0f;
};

} // namespace vecgui
