#pragma once

#include "window.h"

// Engine is designed to isolate entirely from the windowing system,
// as declared in window.h
class Engine {
    private:
        Window* target_window;
    public:
        Engine();
        ~Engine();

        void attach(Window *window);
};
