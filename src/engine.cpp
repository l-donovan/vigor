#include "vigor/engine.h"
#include "vigor/window.h"

Engine::Engine() {
}

Engine::~Engine() {
}

void Engine::attach(Window *window) {
    this->target_window = window;
}
