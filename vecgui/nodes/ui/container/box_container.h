#pragma once

#include "container.h"

namespace vecgui {

enum struct BoxContainerAlignment {
    Begin,
    End,
};

class BoxContainer : public Container {
public:

    void adjust_layout() override;

    void calc_minimum_size() override;

    void set_separation(float new_separation);

    void set_alignment(BoxContainerAlignment new_alignment);

protected:
    BoxContainer() {
    }

    /// Separation between UI children.
    float separation = 8;

    /// Direction for organizing UI children.
    bool horizontal = true;

    BoxContainerAlignment alignment = BoxContainerAlignment::Begin;
};

class HBoxContainer : public BoxContainer {
public:
    HBoxContainer() {
        type = NodeType::HBoxContainer;
        horizontal = true;
    }
};

class VBoxContainer : public BoxContainer {
public:
    VBoxContainer() {
        type = NodeType::VBoxContainer;
        horizontal = false;
    }
};

} // namespace vecgui
