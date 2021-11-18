#include "vigor/engine.h"
#include "vigor/example_layer.h"
#include "vigor/text_layer.h"
#include "vigor/window.h"

#include <iostream>
#include <sstream>
#include <string>

// `_ROOT_DIR` is set via cmake
std::string ROOT_DIR(_ROOT_DIR);

// Create engine, window, shader, and layer objects
Engine engine;
Window window("Vigor", 500, 500);
Shader base_shader(
    ROOT_DIR + "/shaders/base.v.glsl",
    ROOT_DIR + "/shaders/base.f.glsl");
Shader text_shader(
    ROOT_DIR + "/shaders/text.v.glsl",
    ROOT_DIR + "/shaders/text.f.glsl");
ExampleLayer base_layer;
TextLayer text_layer;

void cursor_pos_changed(double x_pos, double y_pos) {
    std::ostringstream text;
    text << "X: " << x_pos << " Y: " << y_pos << std::endl << "Newline test";
    text_layer.set_text(text.str());
    text_layer.set_position(
        x_pos * Window::width,
        (1.0f - y_pos) * Window::height);
}

void window_size_changed(int new_width, int new_height) {
    std::cout << "W: " << new_width << " H: " << new_height << std::endl;
}

int main(int argc, char **argv) {
    // Attach the window to the engine
    engine.attach(&window);

    // Add that layer to the window, using the shader that was
    // created earlier
    window.add_layer(&base_layer, &base_shader);
    window.add_layer(&text_layer, &text_shader);

    // Register some callbacks
    window.register_cursor_pos_fn(cursor_pos_changed);
    window.register_window_size_fn(window_size_changed);

    text_layer.set_position(25.0f, 125.0f);
    text_layer.set_char(0, 0, 'X');
    text_layer.set_char(0, 1, 'Y');
    text_layer.set_char(2, 1, 'Z');

    // Startup the window, compiling the shaders and initializing buffers
    window.startup();

    // Start the window's main loop
    window.main_loop();

    return EXIT_SUCCESS;
}
