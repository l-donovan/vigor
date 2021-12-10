#pragma once

#include "layer.h"
#include "shader.h"

#include <string>
#include <variant>
#include <vector>

#define EVENT_LAYER_ADD 0

enum EventType {
    WindowResize,
    Key,
    CursorPosition,
    WindowResizeRequest,
    LayerUpdateRequest,
    BufferModifyRequest,
    LayerModifyRequest,
};

using variant_t = std::variant<int, float, double, Layer*, Shader*>;

struct Event {
    EventType type;
    std::vector<variant_t> data;
};
