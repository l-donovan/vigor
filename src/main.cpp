#include "vigor/engine.h"
#include "vigor/example_layer.h"
#include "vigor/text_layer.h"
#include "vigor/window.h"

#include <fstream>
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
    text_layer.set_position(x_pos, y_pos);
}

void window_size_changed(int new_width, int new_height) {
    std::cout << "W: " << new_width << " H: " << new_height << std::endl;
}

void key_event(int key, int scancode, int action, int mods) {
    std::cout << "Key: " << static_cast<unsigned int>(scancode) << std::endl;
    text_layer.update();
}

int main(int argc, char **argv) {
    std::string full_text;
    std::string line;

    std::ifstream myfile("test.txt");
    if (myfile.is_open()) {
        while (getline(myfile, line)) {
            full_text.append(line);
            full_text.append("\n");
        }
        myfile.close();
    }

    // Attach the window to the engine
    engine.attach(&window);

    // Add that layer to the window, using the shader that was
    // created earlier
    window.add_layer(&base_layer, &base_shader);
    window.add_layer(&text_layer, &text_shader);

    // Register some callbacks
    window.register_cursor_pos_fn(cursor_pos_changed);
    window.register_window_size_fn(window_size_changed);
    window.register_key_event_fn(key_event);

    // Startup the window, compiling the shaders and initializing buffers
    window.startup();

    // Handle all positioning after startup, once the window's dimensions
    // have been determined
    text_layer.set_text(full_text);
    text_layer.set_position(0.0f, 0.0f);

    // Start the window's main loop
    window.main_loop();

    return EXIT_SUCCESS;
}
