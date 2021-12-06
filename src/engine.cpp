#include "vigor/global.h"
#include "vigor/engine.h"
#include "vigor/event.h"
#include "vigor/window.h"

#include <optional>

Engine::Engine() {
}

Engine::~Engine() {
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
            break;
        case Key:
            this->add_outgoing_event({WindowResizeRequest, {500, 500}});
            break;
        case CursorPosition:
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
