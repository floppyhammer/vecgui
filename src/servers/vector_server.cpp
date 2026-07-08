#include "vector_server.h"

#include "../resources/default_resource.h"
#include "engine.h"

namespace vecgui {

constexpr float STROKE_WIDTH_FOR_PSEUDO_BOLD_TEXT = 1.0;

void VectorServer::init(Pathfinder::Vec2I size,
                        const std::shared_ptr<Pathfinder::Device> &device,
                        const std::shared_ptr<Pathfinder::Queue> &queue,
                        Pathfinder::RenderLevel level) {
    canvas = std::make_shared<Pathfinder::Canvas>(size, device, queue, level);
}

void VectorServer::cleanup() {
    canvas.reset();
}

void VectorServer::set_canvas_size(const Vec2I new_size) {
    const auto new_view_box = RectI({}, new_size).to_f32();
    canvas->get_scene()->set_bounds(new_view_box);
    canvas->get_scene()->set_view_box(new_view_box);
}

void VectorServer::set_dst_texture(const std::shared_ptr<Pathfinder::Texture> &texture) {
    canvas->set_dst_texture(texture);
}

void VectorServer::submit_and_clear() {
    canvas->draw(true);
    canvas->take_scene();
}

std::shared_ptr<Pathfinder::Canvas> VectorServer::get_canvas() const {
    return canvas;
}

float VectorServer::get_global_scale() const {
    return global_scale_;
}

void VectorServer::set_global_scale(float new_scale) {
    global_scale_ = new_scale;
}

void VectorServer::draw_line(Vec2F start, Vec2F end, float width, ColorU color) {
    canvas->save_state();

    Pathfinder::Path2d path;
    path.add_line({start.x, start.y}, {end.x, end.y});

    canvas->set_transform(Pathfinder::Transform2::from_scale(Vec2F(global_scale_, global_scale_)));

    canvas->set_stroke_paint(Pathfinder::Paint::from_color(color));
    canvas->set_line_width(width);
    // canvas->set_line_cap(Pathfinder::LineCap::Round);
    canvas->stroke_path(path);

    canvas->restore_state();
}

void VectorServer::draw_rectangle(const RectF &rect, float line_width, ColorU color, bool fill) {
    canvas->save_state();

    Pathfinder::Path2d path;
    path.add_rect(rect);

    canvas->set_transform(Pathfinder::Transform2::from_scale(Vec2F(global_scale_, global_scale_)));

    if (fill) {
        canvas->set_fill_paint(Pathfinder::Paint::from_color(color));
        canvas->fill_path(path, Pathfinder::FillRule::Winding);
    } else {
        canvas->set_stroke_paint(Pathfinder::Paint::from_color(color));
        canvas->set_line_width(line_width);
        canvas->stroke_path(path);
    }

    canvas->restore_state();
}

void VectorServer::draw_circle(Vec2F center, float radius, float line_width, bool fill, ColorU color) {
    canvas->save_state();

    Pathfinder::Path2d path;
    path.add_circle(center, radius);

    canvas->set_transform(Pathfinder::Transform2::from_scale(Vec2F(global_scale_, global_scale_)));

    if (fill) {
        canvas->set_fill_paint(Pathfinder::Paint::from_color(color));
        canvas->fill_path(path, Pathfinder::FillRule::Winding);
    }
    if (line_width > Pathfinder::FLOAT_EPSILON) { // Ignore too small width
        canvas->set_stroke_paint(Pathfinder::Paint::from_color(color));
        canvas->set_line_width(line_width);
        canvas->stroke_path(path);
    }

    canvas->restore_state();
}

void VectorServer::draw_path(VectorPath &vector_path, Transform2 transform) {
    canvas->save_state();

    auto dpi_scaling_xform = Pathfinder::Transform2::from_scale(Vec2F(global_scale_, global_scale_));

    canvas->set_transform(dpi_scaling_xform * global_transform_offset * transform);

    if (vector_path.fill_color.is_opaque()) {
        canvas->set_fill_paint(Pathfinder::Paint::from_color(vector_path.fill_color));
        canvas->fill_path(vector_path.path2d, Pathfinder::FillRule::Winding);
    }

    if (vector_path.stroke_width > 0) {
        canvas->set_stroke_paint(Pathfinder::Paint::from_color(vector_path.stroke_color));
        canvas->set_line_width(vector_path.stroke_width);
        canvas->stroke_path(vector_path.path2d);
    }

    canvas->restore_state();
}

void VectorServer::draw_raster_image(const RasterImage &image, const Transform2 &transform) {
    canvas->save_state();

    auto dpi_scaling_xform = Pathfinder::Transform2::from_scale(Vec2F(global_scale_, global_scale_));

    canvas->set_transform(dpi_scaling_xform * global_transform_offset * transform);

    auto image_data = image.image_data;

    canvas->draw_image(image_data, RectF({}, Vec2F() + image_data->size.to_f32()));

    canvas->restore_state();
}

void VectorServer::draw_vector_image(VectorImage &image, Transform2 transform) {
    auto dpi_scaling_xform = Pathfinder::Transform2::from_scale(Vec2F(global_scale_, global_scale_));

    for (auto &path : image.get_paths()) {
        draw_path(path, dpi_scaling_xform * transform);
    }

    if (image.get_svg_scene()) {
        canvas->get_scene()->append_scene(*image.get_svg_scene()->get_scene(),
                                          dpi_scaling_xform * global_transform_offset * transform);
    }
}

void VectorServer::draw_render_image(RenderImage &render_image, Transform2 transform) {
    canvas->save_state();

    auto dpi_scaling_xform = Pathfinder::Transform2::from_scale(Vec2F(global_scale_, global_scale_));

    canvas->set_transform(dpi_scaling_xform * global_transform_offset * transform);

    canvas->draw_raw_texture(render_image.get_texture(), RectF({}, render_image.get_size().to_f32()));

    canvas->restore_state();
}

void VectorServer::draw_style_box(const StyleBox &style_box, Vec2F position, Vec2F size, float alpha) {
    if (style_box.border_widths.has_value()) {
        const auto widths = style_box.border_widths.value();
        position.x += widths.left * 0.5f;
        position.y += widths.top * 0.5f;
        size.x += widths.right;
        size.y += widths.bottom;
    } else {
        if (style_box.border_width > 0) {
            const float border_offset = style_box.border_width;
            position.x += border_offset * 0.5f;
            position.y += border_offset * 0.5f;
            size.x -= border_offset;
            size.y -= border_offset;
        }
    }

    auto path = Pathfinder::Path2d();
    if (style_box.corner_radii.has_value()) {
        path.add_rect_with_corners({{}, size}, style_box.corner_radii.value());
    } else {
        path.add_rect({{}, size}, style_box.corner_radius);
    }

    canvas->save_state();

    canvas->set_shadow_color(style_box.shadow_color);
    canvas->set_shadow_blur(style_box.shadow_size);

    const auto dpi_scaling_xform = Pathfinder::Transform2::from_scale(Vec2F(global_scale_, global_scale_));

    const auto transform = Pathfinder::Transform2::from_translation(position);
    canvas->set_transform(dpi_scaling_xform * global_transform_offset * transform);

    canvas->set_fill_paint(Pathfinder::Paint::from_color(style_box.bg_color.apply_alpha(alpha)));
    canvas->fill_path(path, Pathfinder::FillRule::Winding);

    if (style_box.border_widths.has_value()) {
        const auto widths = style_box.border_widths.value();
        if (widths.left > 0) {
            auto line = Pathfinder::Path2d();
            line.add_line({}, Vec2F(0, size.y));
            canvas->set_stroke_paint(Pathfinder::Paint::from_color(style_box.border_color.apply_alpha(alpha)));
            canvas->set_line_width(widths.left);
            canvas->stroke_path(line);
        }
        if (widths.right > 0) {
            auto line = Pathfinder::Path2d();
            line.add_line(Vec2F(0, size.x), size);
            canvas->set_stroke_paint(Pathfinder::Paint::from_color(style_box.border_color.apply_alpha(alpha)));
            canvas->set_line_width(widths.right);
            canvas->stroke_path(line);
        }
        if (widths.top > 0) {
            auto line = Pathfinder::Path2d();
            line.add_line({}, Vec2F(size.x, 0));
            canvas->set_stroke_paint(Pathfinder::Paint::from_color(style_box.border_color.apply_alpha(alpha)));
            canvas->set_line_width(widths.top);
            canvas->stroke_path(line);
        }
        if (widths.bottom > 0) {
            auto line = Pathfinder::Path2d();
            line.add_line(Vec2F(0, size.y), Vec2F(0, size.y));
            canvas->set_stroke_paint(Pathfinder::Paint::from_color(style_box.border_color.apply_alpha(alpha)));
            canvas->set_line_width(widths.bottom);
            canvas->stroke_path(line);
        }
    } else if (style_box.border_width > 0) {
        canvas->set_stroke_paint(Pathfinder::Paint::from_color(style_box.border_color.apply_alpha(alpha)));
        canvas->set_line_width(style_box.border_width);
        canvas->stroke_path(path);
    }

    canvas->restore_state();
}

void VectorServer::draw_style_line(const StyleLine &style_line, const Vec2F &start, const Vec2F &end) {
    auto path = Pathfinder::Path2d();
    path.add_line(start, end);

    canvas->save_state();

    auto dpi_scaling_xform = Pathfinder::Transform2::from_scale(Vec2F(global_scale_, global_scale_));

    canvas->set_transform(dpi_scaling_xform * global_transform_offset);
    canvas->set_stroke_paint(Pathfinder::Paint::from_color(style_line.color));
    canvas->set_line_width(style_line.width);
    canvas->stroke_path(path);

    canvas->restore_state();
}

void VectorServer::draw_glyphs(std::vector<Glyph> &glyphs,
                               std::vector<Vec2F> &glyph_positions,
                               TextStyle text_style,
                               const Transform2 &transform,
                               const RectF &clip_box,
                               float alpha) {
    if (glyphs.size() != glyph_positions.size()) {
        Logger::error("Glyph count mismatches glyph position count!", "vecgui");
        return;
    }

    text_style.color = text_style.color.apply_alpha(alpha);
    text_style.stroke_color = text_style.stroke_color.apply_alpha(alpha);

    canvas->save_state();

    auto dpi_scaling_xform = Pathfinder::Transform2::from_scale(Vec2F(global_scale_, global_scale_));

    // Text clip.
    if (clip_box.is_valid()) {
        auto clip_path = Pathfinder::Path2d();
        clip_path.add_rect(clip_box, 0);
        canvas->set_transform(dpi_scaling_xform * global_transform_offset * transform);
        canvas->clip_path(clip_path, Pathfinder::FillRule::Winding);
    }

    // 0. Draw backgrounds.
    for (int i = 0; i < glyphs.size();) {
        if (glyphs[i].skip_drawing || !glyphs[i].style.background_color.is_visible()) {
            i++;
            continue;
        }

        const TextStyle &style = glyphs[i].style;
        float current_y = glyph_positions[i].y;
        float current_ascent = glyphs[i].ascent;

        // Find the range of glyphs on the same line with the same background style.
        int start = i;
        int end = i + 1;
        while (end < glyphs.size()) {
            if (glyphs[end].skip_drawing) {
                // If it's just a skip_drawing glyph (like space) in the middle of the same line,
                // we can include it in the background if the next glyph has the same style.
                if (end + 1 < glyphs.size() &&
                    glyph_positions[end+1].y == current_y &&
                    glyphs[end+1].style.background_color == style.background_color) {
                    end++;
                    continue;
                }
                break;
            }

            if (glyph_positions[end].y != current_y ||
                !(glyphs[end].style.background_color == style.background_color) ||
                glyphs[end].style.background_corner_radius != style.background_corner_radius ||
                glyphs[end].style.background_padding != style.background_padding) {
                break;
            }
            end++;
        }

        // Calculate the union bounding box of all glyphs in this run.
        RectF union_rect;
        for (int k = start; k < end; ++k) {
            if (glyphs[k].skip_drawing) continue;
            RectF glyph_box = glyphs[k].box + glyph_positions[k];
            union_rect = union_rect.union_rect(glyph_box);
        }

        if (union_rect.is_valid()) {
            auto baseline_xform = Transform2::from_translation({0, current_ascent});
            canvas->set_transform(dpi_scaling_xform * global_transform_offset * transform * baseline_xform);

            if (style.background_padding > 0) {
                union_rect = union_rect.dilate(style.background_padding);
            }

            Pathfinder::Path2d bg_path;
            bg_path.add_rect(union_rect, style.background_corner_radius);

            canvas->set_fill_paint(Pathfinder::Paint::from_color(style.background_color.apply_alpha(alpha)));
            canvas->fill_path(bg_path, Pathfinder::FillRule::Winding);
        }

        i = end;
    }

    // 1. Draw shadows.
    for (int i = 0; i < glyphs.size(); i++) {
        auto &g = glyphs[i];
        auto &p = glyph_positions[i];

        if (g.skip_drawing || !g.style.shadow_color.is_visible()) {
            continue;
        }

        TextStyle style = g.style;
        auto baseline_xform = Transform2::from_translation({0, g.ascent});

        auto shadow_pos = p + style.shadow_offset;
        auto glyph_global_transform =
            dpi_scaling_xform * global_transform_offset * Transform2::from_translation(shadow_pos) * transform * baseline_xform;

        auto skew_xform = Transform2::from_scale({1, 1});
        if (style.italic) {
            skew_xform = Transform2({1, 0, std::tan(-15.f * 3.1415926f / 180.f), 1}, {});
        }

        canvas->set_transform(glyph_global_transform * skew_xform);

        if (style.shadow_radius > 0) {
            canvas->set_shadow_color(style.shadow_color.apply_alpha(alpha));
            canvas->set_shadow_blur(style.shadow_radius);
            canvas->set_shadow_offset({0, 0});
        }

        canvas->set_fill_paint(Pathfinder::Paint::from_color(style.shadow_color.apply_alpha(alpha)));
        canvas->fill_path(g.path, Pathfinder::FillRule::Winding);

        if (style.shadow_radius > 0) {
            canvas->set_shadow_color(ColorU::transparent_black());
            canvas->set_shadow_blur(0);
        }
    }

    // 2. Draw glyph strokes. The strokes go below the fills.
    for (int i = 0; i < glyphs.size(); i++) {
        auto &g = glyphs[i];
        auto &p = glyph_positions[i];

        if (g.emoji || g.skip_drawing) {
            continue;
        }

        TextStyle style = g.style;
        style.stroke_color = style.stroke_color.apply_alpha(alpha);

        auto baseline_xform = Transform2::from_translation({0, g.ascent});

        auto glyph_global_transform =
            dpi_scaling_xform * global_transform_offset * Transform2::from_translation(p) * transform * baseline_xform;

        auto skew_xform = Transform2::from_scale({1, 1});
        if (style.italic) {
            skew_xform = Transform2({1, 0, std::tan(-15.f * 3.1415926f / 180.f), 1}, {});
        }

        canvas->set_transform(glyph_global_transform * skew_xform);

        // Add stroke if needed.
        canvas->set_stroke_paint(Pathfinder::Paint::from_color(style.stroke_color));
        float stroke_width = style.stroke_width;
        if (style.bold) {
            stroke_width += STROKE_WIDTH_FOR_PSEUDO_BOLD_TEXT;
        }
        canvas->set_line_width(stroke_width);
        canvas->set_line_join(Pathfinder::LineJoin::Round);
        canvas->stroke_path(g.path);
    }

    // 3. Draw glyph fills.
    for (int i = 0; i < glyphs.size(); i++) {
        auto &g = glyphs[i];
        auto &p = glyph_positions[i];

        if (g.skip_drawing) {
            continue;
        }

        TextStyle style = g.style;
        style.color = style.color.apply_alpha(alpha);

        auto baseline_xform = Transform2::from_translation({0, g.ascent});

        // No italic for emojis and debug boxes.
        auto glyph_global_transform =
            dpi_scaling_xform * global_transform_offset * Transform2::from_translation(p) * transform * baseline_xform;

        auto skew_xform = Transform2::from_scale({1, 1});
        if (style.italic) {
            skew_xform = Transform2({1, 0, std::tan(-15.f * 3.1415926f / 180.f), 1}, {});
        }

        if (!g.emoji) {
            canvas->set_transform(glyph_global_transform * skew_xform);

            // Add fill.
            canvas->set_fill_paint(Pathfinder::Paint::from_color(style.color));
            canvas->fill_path(g.path, Pathfinder::FillRule::Winding);

            // Use stroke to make a pseudo bold effect.
            if (style.bold) {
                canvas->set_stroke_paint(Pathfinder::Paint::from_color(style.color));
                canvas->set_line_width(STROKE_WIDTH_FOR_PSEUDO_BOLD_TEXT);
                canvas->set_line_join(Pathfinder::LineJoin::Bevel);
                canvas->stroke_path(g.path);
            }
        } else {
            auto svg_scene = std::make_shared<Pathfinder::SvgScene>(g.svg, *canvas);

            // The emoji's svg size is always fixed for a specific font no matter what the font size you set.
            auto svg_size = svg_scene->get_size();
            auto glyph_size = g.box.size();

            auto emoji_scale = Transform2::from_scale(glyph_size / svg_size);

            canvas->get_scene()->append_scene(*(svg_scene->get_scene()), glyph_global_transform * emoji_scale);
        }

        if (style.debug) {
            canvas->set_transform(glyph_global_transform);
            canvas->set_line_width(1);
            auto svg_scene = std::make_shared<Pathfinder::SvgScene>(g.svg, *canvas);

            // The emoji's svg size is always fixed for a specific font no matter what the font size you set.
            auto svg_size = svg_scene->get_size();
            auto glyph_size = g.box.size();

            auto emoji_scale = Transform2::from_scale(glyph_size / svg_size);

            canvas->get_scene()->append_scene(*(svg_scene->get_scene()), glyph_global_transform * emoji_scale);
        }

        if (style.debug) {
            canvas->set_transform(glyph_global_transform);
            canvas->set_line_width(1);

            // Add box.
            // --------------------------------
            Pathfinder::Path2d layout_path;
            layout_path.add_rect(g.box);

            canvas->set_stroke_paint(Pathfinder::Paint::from_color(ColorU::green()));
            canvas->stroke_path(layout_path);
            // --------------------------------

            // Add bbox.
            // --------------------------------
            Pathfinder::Path2d bbox_path;
            bbox_path.add_rect(g.bbox);

            canvas->set_stroke_paint(Pathfinder::Paint::from_color(ColorU::red()));
            canvas->stroke_path(bbox_path);
            // --------------------------------
        }
    }

    canvas->restore_state();
}

std::string replace_all(std::string str, const std::string &from, const std::string &to) {
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.length(), to);
        pos += to.length(); // Advance position to avoid re-finding the newly inserted 'to' string
    }
    return str;
}

std::shared_ptr<Pathfinder::SvgScene> VectorServer::load_svg(const std::string &path, bool override_with_accent_color) {
#ifndef __ANDROID__
    auto bytes = Pathfinder::load_file_as_bytes(path);
#else
    auto bytes = Pathfinder::load_asset(Engine::get_singleton()->asset_manager, path);
#endif

    auto str = std::string(bytes.begin(), bytes.end());

    if (override_with_accent_color) {
        auto default_theme = DefaultResource::get_singleton()->get_default_theme();
        str = replace_all(str, "#000000", default_theme->accent_color.to_hex());
    }

    auto svg_scene = std::make_shared<Pathfinder::SvgScene>(str, *canvas);

    return svg_scene;
}

} // namespace vecgui
