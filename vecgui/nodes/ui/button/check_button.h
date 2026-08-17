#pragma once

#include "button.h"

namespace vecgui {

class CheckButton : public Button {
public:
    CheckButton();

protected:
    void on_ready() override;
};

} // namespace vecgui
