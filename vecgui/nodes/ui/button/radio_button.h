#pragma once

#include "button.h"

namespace vecgui {

class RadioButton : public Button {
public:
    RadioButton();

protected:
    void on_ready() override;
};

} // namespace vecgui
