#pragma once

namespace vecgui {

class Engine;
class VectorServer;
class TextServer;
class InputServer;
class RenderContext;
class TranslationServer;
class DefaultResource;

/// Container for all core services, providing explicit dependency injection.
struct GuiContext {
    Engine* engine = nullptr;
    VectorServer* vector_server = nullptr;
    TextServer* text_server = nullptr;
    InputServer* input_server = nullptr;
    RenderContext* render_context = nullptr;
    TranslationServer* translation_server = nullptr;
    DefaultResource* default_resource = nullptr;
};

} // namespace vecgui
