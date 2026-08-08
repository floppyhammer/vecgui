#pragma once

#include "container.h"

namespace vecgui {

class GridContainer : public Container {
public:
    void adjust_layout() override;

    void calc_minimum_size() override;

    void set_separation(float new_separation);

    void set_column_limit(uint32_t new_limit);

    void set_item_shrinking(bool new_shrinking);

protected:
    uint32_t col_limit = 2;

    /// Separation between UI children.
    float separation = 8;

    float min_row_height;
    float min_col_width;

    bool shrinking = false;
};

} // namespace vecgui
