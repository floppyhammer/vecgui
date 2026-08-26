#include "vector_server.h"

#include "../resources/default_resource.h"
#include "engine.h"

namespace vecgui {

constexpr float STROKE_WIDTH_FOR_PSEUDO_BOLD_TEXT = 1.0;

void VectorServer::init(Pathfinder::Vec2I size,
                        const std::shared_ptr<Pathfinder::Device> &device,
                        const std::shared_ptr<Pathfinder::Queue> &queue,
                        Pathfinder::RenderMode mode) {
    device_ = device;
    queue_ = queue;
    mode_ = mode;

    if (size.x > 0 && size.y > 0) {
        canvas = std::make_shared<Pathfinder::Canvas>(size, device, queue, mode);
    }
}

void VectorServer::destroy() {
    canvas.reset();
    queue_.reset();
    device_.reset();
}

void VectorServer::set_canvas_size(const Vec2I new_size) {
    if (new_size.x <= 0 || new_size.y <= 0) {
        return;
    }

    if (canvas) {
        const auto new_view_box = RectI({}, new_size).to_f32();
        canvas->get_scene()->set_bounds(new_view_box);
        canvas->get_scene()->set_view_box(new_view_box);
    }
}

void VectorServer::set_dst_texture(const std::shared_ptr<Pathfinder::Texture> &texture) {
    if (canvas) {
        canvas->set_dst_texture(texture);
    }
}

void VectorServer::submit_and_clear() {
    if (canvas) {
        canvas->draw(true);
        canvas->take_scene();
    }
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
    draw_style_box(style_box, Pathfinder::Transform2::from_translation(position), size, alpha);
}

void VectorServer::draw_style_box(const StyleBox &style_box, const Transform2 &transform, Vec2F size, float alpha) {
    Vec2F stroke_offset;
    if (style_box.border_widths.has_value()) {
        const auto widths = style_box.border_widths.value();
        stroke_offset.x = widths.left * 0.5f;
        stroke_offset.y = widths.top * 0.5f;
        size.x -= (widths.left + widths.right) * 0.5f;
        size.y -= (widths.top + widths.bottom) * 0.5f;
    } else {
        if (style_box.border_width > 0) {
            const float border_half_width = style_box.border_width * 0.5f;
            stroke_offset = {border_half_width, border_half_width};
            size.x -= style_box.border_width;
            size.y -= style_box.border_width;
        }
    }

    if (size.x <= 0 || size.y <= 0) {
        return;
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

    canvas->set_transform(dpi_scaling_xform * global_transform_offset * transform.translate(stroke_offset));

    // Background.
    canvas->set_fill_paint(Pathfinder::Paint::from_color(style_box.bg_color.apply_alpha(alpha)));
    canvas->fill_path(path, Pathfinder::FillRule::Winding);

    // Border.
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
            line.add_line(Vec2F(size.x, 0), size);
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
            line.add_line(Vec2F(0, size.y), Vec2F(size.x, size.y));
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
    draw_style_line(style_line, Pathfinder::Transform2(), start, end);
}

void VectorServer::draw_style_line(const StyleLine &style_line,
                                   const Transform2 &transform,
                                   const Vec2F &start,
                                   const Vec2F &end) {
    auto path = Pathfinder::Path2d();
    path.add_line(start, end);

    canvas->save_state();

    auto dpi_scaling_xform = Pathfinder::Transform2::from_scale(Vec2F(global_scale_, global_scale_));

    canvas->set_transform(dpi_scaling_xform * global_transform_offset * transform);
    canvas->set_stroke_paint(Pathfinder::Paint::from_color(style_line.color));
    canvas->set_line_width(style_line.width);
    canvas->set_line_cap(Pathfinder::LineCap::Round);
    canvas->stroke_path(path);

    canvas->restore_state();
}

void VectorServer::draw_glyphs(std::vector<Glyph> &glyphs,
                               std::vector<Vec2F> &glyph_positions,
                               const Transform2 &transform,
                               const RectF &clip_box,
                               float opacity,
                               const std::vector<Line> &lines) {
    if (glyphs.size() != glyph_positions.size()) {
        Logger::error("Glyph count mismatches glyph position count!", "vecgui");
        return;
    }

    // Helper: Find which line a glyph belongs to.
    auto get_line_index = [&](int glyph_idx) {
        for (int k = 0; k < (int)lines.size(); ++k) {
            if (glyph_idx >= lines[k].glyph_ranges.start && glyph_idx < lines[k].glyph_ranges.end) {
                return k;
            }
        }
        return -1;
    };

    auto apply_alpha_to_fill = [](std::variant<ColorU, Pathfinder::Gradient> &fill, float alpha) {
        if (alpha >= 1.0f) return;
        if (std::holds_alternative<ColorU>(fill)) {
            std::get<ColorU>(fill) = std::get<ColorU>(fill).apply_alpha(alpha);
        } else {
            auto &grad = std::get<Pathfinder::Gradient>(fill);
            for (auto &stop : grad.get_stops()) {
                stop.color = stop.color.apply_alpha(alpha);
            }
        }
    };

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
        if (glyphs[i].skip_drawing || !glyphs[i].style.background_color.is_visible() ||
            glyphs[i].style.opacity * opacity <= 0.0f) {
            i++;
            continue;
        }

        const TextStyle &style = glyphs[i].style;
        int current_line_idx = get_line_index(i);
        float current_ascent = glyphs[i].ascent;

        // Find the range of glyphs on the same line with the same background style.
        int start = i;
        int end = i + 1;
        bool all_transforms_match = true;
        while (end < glyphs.size()) {
            const auto &ge = glyphs[end];

            // Must be on the same line and have identical background styling.
            if (get_line_index(end) != current_line_idx || !(ge.style.background_color == style.background_color) ||
                ge.style.background_corner_radius != style.background_corner_radius ||
                ge.style.background_expand != style.background_expand) {
                break;
            }

            if (!(ge.style.local_transform == style.local_transform)) {
                all_transforms_match = false;
            }
            end++;
        }

        // Calculate the union bounding box of all glyphs in this run.
        RectF union_rect;
        for (int k = start; k < end; ++k) {
            if (glyphs[k].skip_drawing && k != start && k != end - 1) {
                // For spaces in the middle of a line background, we still want to expand the rect.
                // We use a simple horizontal advance to bridge the gap.
                union_rect =
                    union_rect.union_rect(RectF(glyph_positions[k].x,
                                                glyph_positions[k].y - current_ascent,
                                                glyph_positions[k].x + glyphs[k].x_advance,
                                                glyph_positions[k].y + (glyphs[k].ascent + glyphs[k].descent)));
                continue;
            }
            if (glyphs[k].skip_drawing) continue;
            RectF glyph_box = glyphs[k].box + glyph_positions[k];
            union_rect = union_rect.union_rect(glyph_box);
        }

        if (union_rect.is_valid()) {
            auto baseline_xform = Transform2::from_translation({0, current_ascent});

            // CRITICAL: If transforms within the line differ (e.g. per-word popping),
            // we draw the background strip using IDENTITY transform to keep it static and contiguous.
            Transform2 final_bg_transform = all_transforms_match ? style.local_transform : Transform2();

            canvas->set_transform(dpi_scaling_xform * global_transform_offset * transform * final_bg_transform *
                                  baseline_xform);

            if (style.background_expand > 0) {
                union_rect = union_rect.dilate(style.background_expand);
            }

            Pathfinder::Path2d bg_path;
            bg_path.add_rect(union_rect, style.background_corner_radius);

            canvas->set_fill_paint(
                Pathfinder::Paint::from_color(style.background_color.apply_alpha(opacity * style.opacity)));
            canvas->fill_path(bg_path, Pathfinder::FillRule::Winding);
        }

        i = end;
    }

    // 1. Draw shadows (Line-Batched).
    for (int i = 0; i < glyphs.size();) {
        if (glyphs[i].skip_drawing || !glyphs[i].style.shadow_color.is_visible() ||
            glyphs[i].style.opacity * opacity <= 0.0f) {
            i++;
            continue;
        }

        const TextStyle &style = glyphs[i].style;
        int current_line_idx = get_line_index(i);
        Pathfinder::Path2d combined_shadow_path;

        // Batching: Group consecutive glyphs that share identical shadow properties AND are on the same line.
        int j = i;
        while (j < glyphs.size()) {
            const auto &g = glyphs[j];
            const auto &p = glyph_positions[j];

            if (get_line_index(j) == current_line_idx && g.style.shadow_color == style.shadow_color &&
                g.style.shadow_radius == style.shadow_radius && g.style.shadow_offset == style.shadow_offset &&
                g.style.shadow_strength == style.shadow_strength && g.style.local_transform == style.local_transform &&
                g.style.opacity == style.opacity) {
                if (!g.skip_drawing) {
                    auto shadow_pos = p + style.shadow_offset;
                    auto baseline_xform = Transform2::from_translation({0, g.ascent});
                    auto local_glyph_transform = Transform2::from_translation(shadow_pos) * baseline_xform;

                    auto skew_xform = Transform2::from_scale({1, 1});
                    if (g.style.italic) {
                        skew_xform = Transform2({1, 0, std::tan(-15.f * 3.1415926f / 180.f), 1}, {});
                    }

                    // Merging paths at CPU level is much cheaper than a blur pass on GPU.
                    combined_shadow_path.add_path(g.path, style.local_transform * local_glyph_transform * skew_xform);
                }
                j++;
            } else {
                break;
            }
        }

        // Execute ONE blur/fill operation for the entire batch.
        canvas->save_state();
        // The combined_shadow_path already has local transforms baked in.
        canvas->set_transform(dpi_scaling_xform * global_transform_offset * transform);

        if (style.shadow_color.is_visible()) {
            canvas->set_shadow_color(style.shadow_color.apply_alpha(opacity * style.opacity));
            canvas->set_shadow_strength(style.shadow_strength);
            canvas->set_shadow_blur(style.shadow_radius);
            // Note: offset is already baked into combined_shadow_path transforms.
            canvas->set_shadow_offset({0, 0});

            // Set a transparent color, as we don't need to draw the glyphs itself but only their shadows.
            canvas->set_fill_paint(Pathfinder::Paint::from_color(ColorU::transparent_black()));
            canvas->fill_path(combined_shadow_path, Pathfinder::FillRule::Winding);
        }

        canvas->restore_state();
        i = j;
    }

    // 2. Draw glyph strokes. The strokes go below the fills.
    for (int i = 0; i < glyphs.size(); i++) {
        auto &g = glyphs[i];
        auto &p = glyph_positions[i];

        if (g.emoji || g.skip_drawing || g.style.opacity * opacity <= 0.0f) {
            continue;
        }

        TextStyle style = g.style;
        style.stroke_color = style.stroke_color.apply_alpha(opacity * style.opacity);

        auto baseline_xform = Transform2::from_translation({0, g.ascent});

        auto glyph_global_transform = dpi_scaling_xform * global_transform_offset * transform * style.local_transform *
                                      Transform2::from_translation(p) * baseline_xform;

        auto skew_xform = Transform2::from_scale({1, 1});
        if (style.italic) {
            skew_xform = Transform2({1, 0, std::tan(-15.f * 3.1415926f / 180.f), 1}, {});
        }

        canvas->set_transform(glyph_global_transform * skew_xform);

        // Add stroke if needed.
        canvas->set_stroke_paint(
            Pathfinder::Paint::from_color(style.stroke_color.apply_alpha(opacity * style.opacity)));
        float stroke_width = style.stroke_width;
        if (style.bold) {
            stroke_width += STROKE_WIDTH_FOR_PSEUDO_BOLD_TEXT;
        }
        canvas->set_line_width(stroke_width);
        canvas->set_line_join(Pathfinder::LineJoin::Round);
        canvas->stroke_path(g.path);
    }

    // 3. Draw glyph fills (Batched for Styles & Karaoke).
    for (int i = 0; i < glyphs.size();) {
        auto &g = glyphs[i];
        if (g.skip_drawing || g.style.opacity * opacity <= 0.0f) {
            i++;
            continue;
        }

        TextStyle style = g.style;
        int current_line_idx = get_line_index(i);

        // Grouping: Find consecutive glyphs on the same line with identical styling.
        int j = i + 1;
        while (j < glyphs.size() && glyphs[j].style == style && get_line_index(j) == current_line_idx) {
            j++;
        }

        const auto full_transform = dpi_scaling_xform * global_transform_offset * transform * style.local_transform;

        auto get_paint_for_style = [&](const TextStyle &s, float a, const RectF &bounds) {
            Pathfinder::Paint paint;
            std::visit(
                [&](auto &&arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, ColorU>) {
                        paint = Pathfinder::Paint::from_color(arg.apply_alpha(a * s.opacity));
                    } else if constexpr (std::is_same_v<T, Pathfinder::Gradient>) {
                        auto grad_copy = arg;

                        // Local Normalization:
                        // Map the unit square [0, 1] to the actual bounding box of the text span.
                        auto norm_xform =
                            Transform2::from_translation(bounds.origin()) * Transform2::from_scale(bounds.size());
                        auto total_xform = full_transform * norm_xform;

                        std::visit(
                            [&](auto &&geom) {
                                using GT = std::decay_t<decltype(geom)>;
                                if constexpr (std::is_same_v<GT, Pathfinder::GradientLinear>) {
                                    auto &lin = std::get<Pathfinder::GradientLinear>(grad_copy.geometry);
                                    lin.line = lin.line.apply_transform(total_xform);
                                } else if constexpr (std::is_same_v<GT, Pathfinder::GradientRadial>) {
                                    auto &rad = std::get<Pathfinder::GradientRadial>(grad_copy.geometry);
                                    rad.line = rad.line.apply_transform(total_xform);
                                }
                            },
                            arg.geometry);
                        for (auto &stop : grad_copy.get_stops()) {
                            stop.color = stop.color.apply_alpha(a * s.opacity);
                        }
                        paint = Pathfinder::Paint::from_gradient(grad_copy);
                    }
                },
                s.fill);
            return paint;
        };

        Pathfinder::Path2d combined_word_path;
        bool is_karaoke = style.karaoke_progress >= 0.0f;
        RectF batch_bounds;
        bool first_glyph = true;

        // Build a combined path for this styling run to apply gradient correctly across all letters.
        for (int k = i; k < j; k++) {
            if (glyphs[k].skip_drawing || glyphs[k].emoji) {
                continue;
            }

            auto baseline_xform = Transform2::from_translation({0, glyphs[k].ascent});
            auto local_glyph_transform = Transform2::from_translation(glyph_positions[k]) * baseline_xform;

            auto skew_xform = Transform2::from_scale({1, 1});
            if (glyphs[k].style.italic) {
                skew_xform = Transform2({1, 0, std::tan(-15.f * 3.1415926f / 180.f), 1}, {});
            }

            combined_word_path.add_path(glyphs[k].path, local_glyph_transform * skew_xform);

            // Calculate bounding box for local normalization.
            RectF glyph_rect = glyphs[k].box + glyph_positions[k];
            if (first_glyph) {
                batch_bounds = glyph_rect;
                first_glyph = false;
            } else {
                batch_bounds = batch_bounds.union_rect(glyph_rect);
            }
        }

        canvas->save_state();
        canvas->set_transform(full_transform);

        if (is_karaoke) {
            float word_min_x = 1e9f;
            float word_max_x = -1e9f;
            for (int k = i; k < j; k++) {
                word_min_x = std::min(word_min_x, glyph_positions[k].x);
                word_max_x = std::max(word_max_x, glyph_positions[k].x + glyphs[k].x_advance);
            }

            // Define the gradient line in Label-Local space.
            // Using glyph_positions[i].y as a reference Y for the horizontal gradient.
            Vec2F p0 = {word_min_x, glyph_positions[i].y};
            Vec2F p1 = {word_max_x, glyph_positions[i].y};

            // RTL support: If the script is Arabic or Hebrew, the progressive coloring should go from right to left.
            if (glyphs[i].script == Script::Arabic || glyphs[i].script == Script::Hebrew) {
                std::swap(p0, p1);
            }

            // CRITICAL: Transform the gradient line to SCENE space to match the rendered path.
            // This ensures the gradient "sticks" to the text as it moves/scales.
            auto grad_line = Pathfinder::LineSegmentF(p0, p1).apply_transform(full_transform);
            auto gradient = Pathfinder::Gradient::linear(grad_line);

            float prg = std::clamp(style.karaoke_progress, 0.0f, 1.0f);

            // 4-stop chain for maximum robustness across different GPU drivers.
            gradient.add_color_stop(style.get_fill_color().apply_alpha(opacity * style.opacity), prg);
            gradient.add_color_stop(style.get_fill_color().apply_alpha(opacity * style.opacity), 1.0f);

            gradient.add_color_stop(style.karaoke_reached_color.apply_alpha(opacity * style.opacity), 0.0f);
            gradient.add_color_stop(style.karaoke_reached_color.apply_alpha(opacity * style.opacity), prg);

            canvas->set_fill_paint(Pathfinder::Paint::from_gradient(gradient));
            canvas->fill_path(combined_word_path, Pathfinder::FillRule::Winding);
        } else if (style.gradient_mapping_mode == GradientMappingMode::Span) {
            canvas->set_fill_paint(get_paint_for_style(style, opacity, batch_bounds));
            canvas->fill_path(combined_word_path, Pathfinder::FillRule::Winding);
        } else {
            // Per-glyph fill.
            for (int k = i; k < j; k++) {
                if (glyphs[k].skip_drawing || glyphs[k].emoji) continue;
                auto baseline_xform = Transform2::from_translation({0, glyphs[k].ascent});
                auto local_glyph_transform = Transform2::from_translation(glyph_positions[k]) * baseline_xform;
                auto skew_xform = Transform2::from_scale({1, 1});
                if (glyphs[k].style.italic) {
                    skew_xform = Transform2({1, 0, std::tan(-15.f * 3.1415926f / 180.f), 1}, {});
                }

                canvas->save_state();
                canvas->set_transform(full_transform * local_glyph_transform * skew_xform);
                canvas->set_fill_paint(
                    get_paint_for_style(glyphs[k].style, opacity, glyphs[k].box + glyph_positions[k]));
                canvas->fill_path(glyphs[k].path, Pathfinder::FillRule::Winding);
                canvas->restore_state();
            }
        }

        // Handle Pseudo-Bold and Emojis within the batch (they don't support simple path-level gradients well).
        for (int k = i; k < j; k++) {
            auto &gk = glyphs[k];
            if (gk.skip_drawing) continue;

            auto baseline_xform = Transform2::from_translation({0, gk.ascent});
            auto glyph_global_transform =
                full_transform * Transform2::from_translation(glyph_positions[k]) * baseline_xform;

            auto skew_xform = Transform2::from_scale({1, 1});
            if (gk.style.italic) {
                skew_xform = Transform2({1, 0, std::tan(-15.f * 3.1415926f / 180.f), 1}, {});
            }

            if (gk.emoji) {
                // First try COLR layers.
                if (!gk.layers.empty()) {
                    canvas->save_state();
                    canvas->set_transform(glyph_global_transform * skew_xform);
                    for (auto &layer : gk.layers) {
                        std::visit(
                            [&](auto &&arg) {
                                using T = std::decay_t<decltype(arg)>;
                                if constexpr (std::is_same_v<T, ColorU>) {
                                    ColorU fill_color = arg;
                                    if (fill_color == ColorU::transparent_black()) {
                                        // Use text foreground color.
                                        fill_color = gk.style.get_fill_color().apply_alpha(opacity * gk.style.opacity);
                                    } else {
                                        fill_color = fill_color.apply_alpha(opacity * gk.style.opacity);
                                    }
                                    canvas->set_fill_paint(Pathfinder::Paint::from_color(fill_color));
                                } else if constexpr (std::is_same_v<T, Pathfinder::Gradient>) {
                                    auto grad_copy = arg;
                                    for (auto &stop : grad_copy.get_stops()) {
                                        stop.color = stop.color.apply_alpha(opacity * gk.style.opacity);
                                    }

                                    // Map gradient geometry to the scene space.
                                    std::visit(
                                        [&](auto &&geom) {
                                            using GT = std::decay_t<decltype(geom)>;
                                            if constexpr (std::is_same_v<GT, Pathfinder::GradientLinear>) {
                                                auto &lin = std::get<Pathfinder::GradientLinear>(grad_copy.geometry);
                                                lin.line =
                                                    lin.line.apply_transform(glyph_global_transform * skew_xform);
                                            } else if constexpr (std::is_same_v<GT, Pathfinder::GradientRadial>) {
                                                auto &rad = std::get<Pathfinder::GradientRadial>(grad_copy.geometry);
                                                rad.line =
                                                    rad.line.apply_transform(glyph_global_transform * skew_xform);
                                                // Radius scaling (approximate for non-uniform scale).
                                                rad.radii = rad.radii * glyph_global_transform.m11();
                                            }
                                        },
                                        arg.geometry);

                                    canvas->set_fill_paint(Pathfinder::Paint::from_gradient(grad_copy));
                                }
                            },
                            layer.fill);

                        canvas->fill_path(layer.path, Pathfinder::FillRule::Winding);
                    }
                    canvas->restore_state();
                }
            } else if (gk.style.bold) {
                canvas->set_transform(glyph_global_transform * skew_xform);
                RectF stroke_bounds = (gk.style.gradient_mapping_mode == GradientMappingMode::Span)
                                          ? batch_bounds
                                          : (gk.box + glyph_positions[k]);
                canvas->set_stroke_paint(get_paint_for_style(gk.style, opacity, stroke_bounds));
                canvas->set_line_width(STROKE_WIDTH_FOR_PSEUDO_BOLD_TEXT);
                canvas->set_line_join(Pathfinder::LineJoin::Round);
                canvas->stroke_path(gk.path);
            }

            if (gk.style.debug) {
                canvas->set_transform(glyph_global_transform);
                canvas->set_line_width(1);
                Pathfinder::Path2d layout_path;
                layout_path.add_rect(gk.box);
                canvas->set_stroke_paint(Pathfinder::Paint::from_color(ColorU::green()));
                canvas->stroke_path(layout_path);
                Pathfinder::Path2d bbox_path;
                bbox_path.add_rect(gk.bbox);
                canvas->set_stroke_paint(Pathfinder::Paint::from_color(ColorU::red()));
                canvas->stroke_path(bbox_path);
            }
        }

        canvas->restore_state();
        i = j;
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

std::shared_ptr<Pathfinder::SvgScene> VectorServer::load_svg(const GuiContext *context,
                                                             const std::string &path,
                                                             bool override_with_accent_color) {
    if (!context || !context->engine) {
        return nullptr;
    }

#ifndef __ANDROID__
    auto bytes = Pathfinder::load_file_as_bytes(path);
#else
    auto bytes = Pathfinder::load_asset(context->engine->asset_manager, path);
#endif

    auto str = std::string(bytes.begin(), bytes.end());

    if (override_with_accent_color) {
        if (context->default_resource) {
            auto default_theme = context->default_resource->get_default_theme();
            str = replace_all(str, "#000000", default_theme->accent_color.to_hex());
        }
    }

    auto svg_scene = std::make_shared<Pathfinder::SvgScene>(str, *canvas);

    return svg_scene;
}

} // namespace vecgui
