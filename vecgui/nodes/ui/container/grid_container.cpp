#include "grid_container.h"
#include <algorithm>
#include <vector>

namespace vecgui {

void GridContainer::adjust_layout() {
    if (children.empty() || col_limit == 0) {
        cached_col_widths.clear();
        cached_row_heights.clear();
        return;
    }

    std::vector<NodeUi *> ui_children = get_visible_ui_children();
    if (ui_children.empty()) {
        cached_col_widths.clear();
        cached_row_heights.clear();
        return;
    }

    uint32_t col_num = std::min((uint32_t)ui_children.size(), col_limit);
    if (col_num == 0) col_num = 1;
    uint32_t row_num = (ui_children.size() + col_num - 1) / col_num;

    std::vector<float> col_min_widths(col_num, min_col_width);
    std::vector<float> row_min_heights(row_num, min_row_height);

    std::vector<bool> col_expanding(col_num, false);
    std::vector<bool> row_expanding(row_num, false);

    // 1. Calculate min widths for columns, min heights for rows, and expansion flags
    for (int child_idx = 0; child_idx < ui_children.size(); child_idx++) {
        auto &child = ui_children[child_idx];
        if (!child->get_visibility() || !child->is_ui_node()) {
            continue;
        }

        int row_idx = child_idx / col_num;
        int col_idx = child_idx % col_num;

        auto child_min_size = child->get_effective_minimum_size();
        col_min_widths[col_idx] = std::max(col_min_widths[col_idx], child_min_size.x);
        row_min_heights[row_idx] = std::max(row_min_heights[row_idx], child_min_size.y);

        if (child->container_sizing.expand_h()) {
            col_expanding[col_idx] = true;
        }
        if (child->container_sizing.expand_v()) {
            row_expanding[row_idx] = true;
        }
    }

    float min_total_w = 0;
    for (float w : col_min_widths) min_total_w += w;
    if (col_num > 1) min_total_w += separation * (col_num - 1);

    float min_total_h = 0;
    for (float h : row_min_heights) min_total_h += h;
    if (row_num > 1) min_total_h += separation * (row_num - 1);

    Vec2F effective_min_size = {min_total_w, min_total_h};
    float available_w = size.x - effective_min_size.x;
    float available_h = size.y - effective_min_size.y;
    size = size.max(effective_min_size);

    // 2. Distribute available_w to expanding columns using water-level algorithm
    std::vector<float> col_widths = col_min_widths;
    std::vector<float> expanding_col_min_widths;
    std::vector<int> expanding_col_indices;
    for (int c = 0; c < col_num; c++) {
        if (col_expanding[c]) {
            expanding_col_min_widths.push_back(col_min_widths[c]);
            expanding_col_indices.push_back(c);
        }
    }

    if (!expanding_col_min_widths.empty() && available_w > 0) {
        std::vector<float> sorted_mins = expanding_col_min_widths;
        std::sort(sorted_mins.begin(), sorted_mins.end());
        float running_sum = 0;
        float target_size = sorted_mins[0];
        for (size_t k = 0; k < sorted_mins.size(); ++k) {
            running_sum += sorted_mins[k];
            float t = (running_sum + available_w) / (float)(k + 1);
            if (k + 1 < sorted_mins.size()) {
                if (t <= sorted_mins[k + 1]) {
                    target_size = t;
                    break;
                }
            } else {
                target_size = t;
            }
        }
        for (int c : expanding_col_indices) {
            col_widths[c] = std::max(col_min_widths[c], target_size);
        }
    }

    // 3. Distribute available_h to expanding rows using water-level algorithm
    std::vector<float> row_heights = row_min_heights;
    std::vector<float> expanding_row_min_heights;
    std::vector<int> expanding_row_indices;
    for (int r = 0; r < row_num; r++) {
        if (row_expanding[r]) {
            expanding_row_min_heights.push_back(row_min_heights[r]);
            expanding_row_indices.push_back(r);
        }
    }

    if (!expanding_row_min_heights.empty() && available_h > 0) {
        std::vector<float> sorted_mins = expanding_row_min_heights;
        std::sort(sorted_mins.begin(), sorted_mins.end());
        float running_sum = 0;
        float target_size = sorted_mins[0];
        for (size_t k = 0; k < sorted_mins.size(); ++k) {
            running_sum += sorted_mins[k];
            float t = (running_sum + available_h) / (float)(k + 1);
            if (k + 1 < sorted_mins.size()) {
                if (t <= sorted_mins[k + 1]) {
                    target_size = t;
                    break;
                }
            } else {
                target_size = t;
            }
        }
        for (int r : expanding_row_indices) {
            row_heights[r] = std::max(row_min_heights[r], target_size);
        }
    }

    cached_col_widths = col_widths;
    cached_row_heights = row_heights;

    // 4. Position and size each child within its cell based on container_sizing
    for (int child_idx = 0; child_idx < ui_children.size(); child_idx++) {
        auto &child = ui_children[child_idx];
        if (!child->get_visibility() || !child->is_ui_node()) {
            continue;
        }

        int row_idx = child_idx / col_num;
        int col_idx = child_idx % col_num;

        float cell_x = 0;
        for (int c = 0; c < col_idx; c++) {
            cell_x += col_widths[c] + separation;
        }

        float cell_y = 0;
        for (int r = 0; r < row_idx; r++) {
            cell_y += row_heights[r] + separation;
        }

        float cell_w = col_widths[col_idx];
        float cell_h = row_heights[row_idx];

        auto child_min_size = child->get_effective_minimum_size();

        float final_w = child_min_size.x;
        float final_h = child_min_size.y;
        float pos_x = cell_x;
        float pos_y = cell_y;

        // Horizontal sizing
        switch (child->container_sizing.flag_h) {
            case ContainerSizingFlag::NoExpand:
            case ContainerSizingFlag::Fill:
                final_w = cell_w;
                pos_x = cell_x;
                break;
            case ContainerSizingFlag::ShrinkStart:
                final_w = child_min_size.x;
                pos_x = cell_x;
                break;
            case ContainerSizingFlag::ShrinkCenter:
                final_w = child_min_size.x;
                pos_x = cell_x + (cell_w - child_min_size.x) * 0.5f;
                break;
            case ContainerSizingFlag::ShrinkEnd:
                final_w = child_min_size.x;
                pos_x = cell_x + cell_w - child_min_size.x;
                break;
        }

        // Vertical sizing
        switch (child->container_sizing.flag_v) {
            case ContainerSizingFlag::NoExpand:
            case ContainerSizingFlag::Fill:
                final_h = cell_h;
                pos_y = cell_y;
                break;
            case ContainerSizingFlag::ShrinkStart:
                final_h = child_min_size.y;
                pos_y = cell_y;
                break;
            case ContainerSizingFlag::ShrinkCenter:
                final_h = child_min_size.y;
                pos_y = cell_y + (cell_h - child_min_size.y) * 0.5f;
                break;
            case ContainerSizingFlag::ShrinkEnd:
                final_h = child_min_size.y;
                pos_y = cell_y + cell_h - child_min_size.y;
                break;
        }

        child->set_position({pos_x, pos_y});
        child->set_size({final_w, final_h});
    }

    // Set container self size
    float total_w = 0;
    for (float w : col_widths) total_w += w;
    if (col_num > 1) total_w += separation * (col_num - 1);

    float total_h = 0;
    for (float h : row_heights) total_h += h;
    if (row_num > 1) total_h += separation * (row_num - 1);

    set_size({total_w, total_h});
}

void GridContainer::calc_minimum_size() {
    std::vector<NodeUi *> ui_children = get_visible_ui_children();
    if (ui_children.empty() || col_limit == 0) {
        calculated_minimum_size = {0, 0};
        min_col_width = 0;
        min_row_height = 0;
        return;
    }

    uint32_t col_num = std::min((uint32_t)ui_children.size(), col_limit);
    if (col_num == 0) col_num = 1;
    uint32_t row_num = (ui_children.size() + col_num - 1) / col_num;

    std::vector<float> col_min_widths(col_num, min_col_width);
    std::vector<float> row_min_heights(row_num, min_row_height);

    for (int child_idx = 0; child_idx < ui_children.size(); child_idx++) {
        auto ui_child = ui_children[child_idx];
        if (!ui_child->get_visibility() || !ui_child->is_ui_node()) {
            continue;
        }

        int row_idx = child_idx / col_num;
        int col_idx = child_idx % col_num;

        auto child_min_size = ui_child->get_effective_minimum_size();
        col_min_widths[col_idx] = std::max(col_min_widths[col_idx], child_min_size.x);
        row_min_heights[row_idx] = std::max(row_min_heights[row_idx], child_min_size.y);
    }

    float total_w = 0;
    for (float w : col_min_widths) total_w += w;
    if (col_num > 1) total_w += separation * (col_num - 1);

    float total_h = 0;
    for (float h : row_min_heights) total_h += h;
    if (row_num > 1) total_h += separation * (row_num - 1);

    calculated_minimum_size = {total_w, total_h};
}

void GridContainer::draw() {
    Container::draw();

    if (!visible_ || !show_grid_lines || separation <= 0) {
        return;
    }

    auto context = get_context();
    if (!context) {
        return;
    }
    auto vector_server = context->vector_server;
    auto global_pos = get_global_position();

    uint32_t col_num = cached_col_widths.size();
    uint32_t row_num = cached_row_heights.size();

    // Draw vertical divider lines between columns in the separation gap
    float current_x = global_pos.x;
    for (size_t c = 0; c + 1 < col_num; c++) {
        current_x += cached_col_widths[c];
        float line_x = current_x + separation * 0.5f;

        float total_h = 0;
        for (float h : cached_row_heights) total_h += h;
        if (row_num > 1) total_h += separation * (row_num - 1);

        vector_server->draw_line(
            {line_x, global_pos.y},
            {line_x, global_pos.y + total_h},
            grid_line_width,
            grid_line_color
        );
        current_x += separation;
    }

    // Draw horizontal divider lines between rows in the separation gap
    float current_y = global_pos.y;
    for (size_t r = 0; r + 1 < row_num; r++) {
        current_y += cached_row_heights[r];
        float line_y = current_y + separation * 0.5f;

        float total_w = 0;
        for (float w : cached_col_widths) total_w += w;
        if (col_num > 1) total_w += separation * (col_num - 1);

        vector_server->draw_line(
            {global_pos.x, line_y},
            {global_pos.x + total_w, line_y},
            grid_line_width,
            grid_line_color
        );
        current_y += separation;
    }
}

void GridContainer::set_separation(float new_separation) {
    if (separation == new_separation) {
        return;
    }
    separation = new_separation;
}

void GridContainer::set_column_limit(uint32_t new_limit) {
    col_limit = new_limit;
}

void GridContainer::set_item_shrinking(bool new_shrinking) {
    shrinking = new_shrinking;
}

void GridContainer::set_show_grid_lines(bool show) {
    show_grid_lines = show;
}

void GridContainer::set_grid_line_color(ColorU color) {
    grid_line_color = color;
}

void GridContainer::set_grid_line_width(float width) {
    grid_line_width = width;
}

} // namespace vecgui
