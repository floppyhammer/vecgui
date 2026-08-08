#include <cstdint>
#include <memory>
#include <string>

#include "../common/utils.h"

#ifndef __APPLE__

namespace vecgui {

std::string get_asset_dir(const std::string &relative_path) {
#ifdef __ANDROID__
    return relative_path;
#endif

    // AppImage specific.
    const auto app_dir = getenv("APPDIR");

    std::string asset_dir;
    if (app_dir) {
        Logger::debug("Got the app directory: " + std::string(app_dir), "vecgui");
        asset_dir = std::string(app_dir) + "/assets/";
    } else {
        Logger::debug("Failed to get the app directory, will use relative path", "vecgui");
        asset_dir = "assets/";
    }

    return asset_dir + relative_path;
}

} // namespace vecgui

#endif
