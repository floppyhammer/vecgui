#pragma once

#include <chrono>

namespace vecgui {

class Engine {
public:
    Engine();

    void tick();

    double get_dt() const;

    double get_elapsed() const;

    float get_fps();

    int32_t get_fps_int();

    void *asset_manager{};

private:
    std::chrono::time_point<std::chrono::steady_clock> last_time_updated_fps;

    float smoothed_fps = 0.0f;

    double elapsed = 0;
    double dt = 0;
};

} // namespace vecgui
