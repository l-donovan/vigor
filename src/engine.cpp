#include "vigor/global.h"
#include "vigor/engine.h"
#include "vigor/event.h"
#include "vigor/shader.h"
#include "vigor/example_layer.h"
#include "vigor/text_buffer.h"
#include "vigor/text_layer.h"
#include "vigor/window.h"

#include <optional>
#include <string>

// `_ROOT_DIR` is set via cmake
std::string ROOT_DIR(_ROOT_DIR);

Shader base_shader(
    ROOT_DIR + "/shaders/base.v.glsl",
    ROOT_DIR + "/shaders/base.f.glsl");
Shader text_shader(
    ROOT_DIR + "/shaders/text.v.glsl",
    ROOT_DIR + "/shaders/text.f.glsl");
ExampleLayer base_layer;
TextLayer text_layer;

bool file_write_callback(std::streambuf::int_type c) {
    return true;
}

TextBuffer buffer(file_write_callback);

Engine::Engine() {
}

Engine::~Engine() {
}

void Engine::pre_window_startup() {
    // We have to send some events to the window to setup our layers and shaders
    this->add_outgoing_event({LayerModifyRequest, {
        EVENT_LAYER_ADD,
        &base_layer,
        &base_shader
    }});

    this->add_outgoing_event({LayerModifyRequest, {
        EVENT_LAYER_ADD,
        &text_layer,
        &text_shader
    }});

    // Load some lorem ipsum text and bind the text buffer to our text layer
    buffer.load_file(ROOT_DIR + "/test.txt");
    text_layer.bind_text_buffer(&buffer);
}

// This must be called after the window has had its `startup` called
void Engine::post_window_startup() {
#ifdef _WIN32
    text_layer.set_font("C:\\Windows\\Fonts\\IBMPlexMono-Regular.ttf", 24);
#elif __APPLE__
    text_layer.set_font("/Users/ldonovan/Library/Fonts/Blex Mono Nerd Font Complete Mono-1.ttf", 24);
#else
    text_layer.set_font("/home/luke/.local/share/fonts/Blex Mono Nerd Font Complete Mono.ttf", 24);
#endif

    text_layer.set_position(0.0f, 0.0f);
}

void Engine::handle_key_event(int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        text_layer.set_start_line(text_layer.get_start_line() + 1);
        this->add_outgoing_event({LayerUpdateRequest, {}});
    } else if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        text_layer.set_start_line(text_layer.get_start_line() - 1);
        this->add_outgoing_event({LayerUpdateRequest, {}});
    }
}

void Engine::process_events() {
    // This is where the logic is actually handled
    // 1. Process incoming events from the window
    // 2. Send outgoing events to the window

    std::optional<Event> event;
    while ((event = this->pop_incoming_event()).has_value()) {
        switch (event->type) {
        case WindowResize:
            PLOGI << "Got window resize event";
            PLOGI << "W: " << std::get<int>(event->data[0]) << ", H: " << std::get<int>(event->data[1]);
            this->add_outgoing_event({LayerUpdateRequest, {}});
            break;
        case Key:
            this->handle_key_event(
                std::get<int>(event->data[0]),
                std::get<int>(event->data[1]),
                std::get<int>(event->data[2]),
                std::get<int>(event->data[3])
            );
            break;
        case CursorPosition:
            text_layer.set_position(
                std::get<double>(event->data[0]),
                std::get<double>(event->data[1])
            );
            break;
        default:
            PLOGE << "Got unknown event type";
            break;
        }
    }
}

std::optional<Event> Engine::pop_incoming_event() {
    if (this->incoming_event_queue.empty())
        return {};

    Event event = this->incoming_event_queue.front();
    this->incoming_event_queue.pop();
    return event;
}

void Engine::add_outgoing_event(Event event) {
    this->outgoing_event_queue.push(event);
}

std::optional<Event> Engine::pop_outgoing_event() {
    if (this->outgoing_event_queue.empty())
        return {};

    Event event = this->outgoing_event_queue.front();
    this->outgoing_event_queue.pop();
    return event;
}

void Engine::add_incoming_event(Event event) {
    this->incoming_event_queue.push(event);
}
