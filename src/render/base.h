#pragma once

#include "vecgui_config.h"
#include <pathfinder/prelude.h>

#if !defined(ANDROID) && defined(VECGUI_USE_WINDOW)
    #define GLFW_INCLUDE_NONE
    #include <GLFW/glfw3.h>
#endif
