#pragma once

#include "container.h"
#include <vector>

namespace vecgui {

class GridContainer : public Container {
public:
    void adjust_layout() override;

    void calc_minimum_size() override;

    void draw() override;

    void set_separation(float new_separation);

    void set_column_limit(uint32_t new_limit);

    void set_item_shrinking(bool new_shrinking);

    void set_show_grid_lines(bool show);
    void set_grid_line_color(ColorU color);
    void set_grid_line_width(float width);

protected:
    uint32_t col_limit = 2;

    /// Separation between UI children.
    float separation = 8;

    float min_row_height = 0;
    float min_col_width = 0;

    bool shrinking = false;

    bool show_grid_lines = true;
    ColorU grid_line_color = {255, 255, 255, 30};
    float grid_line_width = 1.0f;

    std::vector<float> cached_col_widths;
    std::vector<float> cached_row_heights;
};

} // namespace vecgui
