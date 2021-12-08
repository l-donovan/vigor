#include "vigor/global.h"
#include "vigor/engine.h"
#include "vigor/example_layer.h"
#include "vigor/text_buffer.h"
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

bool file_write_callback(std::streambuf::int_type c) {
    PLOGD << "Callback got: " << static_cast<char>(c);
    return true;
}

TextBuffer test_file(file_write_callback);

void cursor_pos_changed(double x_pos, double y_pos) {
    text_layer.set_position(x_pos, y_pos);
}

void window_size_changed(int new_width, int new_height) {
    //text_layer.update();
}

int main(int argc, char **argv) {
    // The process for resizing the window when a key is pressed:
    // 1. glfw gets keypress
    // 2. glfw sends the keypress to the registered handler
    // 3. the handler sends a keypress event to the engine
    // 4. the engine processes the event
    // 5. the engine sends a resize event to the window
    // 6. the engine processes the event and asks glfw to resize the window

    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(plog::debug, &consoleAppender);

    std::ifstream t;
    t.open("C:\\Users\\ladbu\\projects\\vigor\\test.txt");
    std::stringstream buffer;
    buffer << t.rdbuf();
    text_layer.bind_buffer(&test_file);

    // Attach the window to the engine
    // (it's actually happening the other way around behind the scenes)
    window.attach_to(engine);

    // Add that layer to the window, using the shader that was
    // created earlier
    window.add_layer(&base_layer, &base_shader);
    window.add_layer(&text_layer, &text_shader);

    // Register some callbacks
    window.register_cursor_pos_fn(cursor_pos_changed);
    window.register_window_size_fn(window_size_changed);

    // Startup the window, compiling the shaders and initializing buffers
    window.startup();

    // Handle all positioning after startup, once the window's dimensions
    // have been determined
    text_layer.set_font("C:\\Windows\\Fonts\\IBMPlexMono-Regular.ttf", 24);
    text_layer.set_text("test\ttest" + buffer.str());
    text_layer.set_position(0.0f, 0.0f);

    // Start the window's main loop
    window.main_loop();

    //test_file.close();

    return EXIT_SUCCESS;
}
