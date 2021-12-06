#pragma once

#include <string>
#include <variant>
#include <vector>

enum EventType {
    WindowResize,
    Key,
    CursorPosition,
    WindowResizeRequest,
};

using variant_t = std::variant<int, float, double>;

struct Event {
    EventType type;
    std::vector<variant_t> data;
};
