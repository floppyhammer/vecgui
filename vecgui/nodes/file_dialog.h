#pragma once

#include <optional>

#include "node.h"

namespace vecgui {

class FileDialog final : public Node {
public:
    std::optional<std::string> show();

    void set_default_path(const std::string& default_path);

protected:
    std::string default_path_;
};

} // namespace vecgui
