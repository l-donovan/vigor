#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <plog/Log.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "vigor/global.h"
#include "vigor/engine.h"
#include "vigor/event.h"
#include "vigor/layer.h"
#include "vigor/shader.h"
#include "vigor/window.h"

// `_ROOT_DIR` is set via cmake
//std::string ROOT_DIR(_ROOT_DIR);

using std::string;

#include <chrono>
using namespace std::chrono;

Window::Window(string window_title, int initial_width, int initial_height) {
    this->window_title = window_title;
    this->initial_width = initial_width;
    this->initial_height = initial_height;
}

Window::~Window() {
}

void (*Window::cursor_pos)(double x_pos, double y_pos) = NULL;
void (*Window::window_size)(int width, int height) = NULL;
void (*Window::key_event)(int key, int scancode, int action, int mods) = NULL;
Engine Window::engine = Engine();

int Window::width = 0;
int Window::height = 0;

int fps_count = 0;
float fps_avg = 0.0f;
int fps_samples = 30;

void Window::attach_to(Engine engine) {
    Window::engine = engine;
}

void Window::global_cursor_pos_callback(GLFWwindow *window, double x_pos, double y_pos) {
    double x_adj = x_pos / Window::width;
    double y_adj = y_pos / Window::height;

    if (x_adj < 0.0 || x_adj > 1.0 || y_adj < 0.0 || y_adj > 1.0)
        return;

    Window::cursor_pos(x_adj, y_adj);
    Window::engine.add_incoming_event({CursorPosition, {x_pos, y_pos}});
}

void Window::global_window_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);

    Window::width = width;
    Window::height = height;

    Window::window_size(width, height); // TODO: These should eventually be removed
    Window::engine.add_incoming_event({WindowResize, {width, height}});
}

void Window::global_key_event_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else {
        Window::key_event(key, scancode, action, mods); // TODO: These should eventually be removed
        Window::engine.add_incoming_event({Key, {key, scancode, action, mods}});
    }
}

void Window::register_cursor_pos_fn(void (*fp)(double x_pos, double y_pos)) {
    Window::cursor_pos = fp;
}

void Window::register_window_size_fn(void (*fp)(int width, int height)) {
    Window::window_size = fp;
}

void Window::register_key_event_fn(void (*fp)(int key, int scancode, int action, int mods)) {
    Window::key_event = fp;
}

static void glfw_error_callback(int error, const char *description) {
    PLOGE << "Error: " << description;
}

bool Window::startup() {
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit()) {
        PLOGE << "Error: glfwInit";
        return false;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    this->win = glfwCreateWindow(
            this->initial_width, this->initial_height,
            this->window_title.c_str(),
            nullptr, nullptr);

    if (!this->win) {
        glfwTerminate();
        PLOGE << "Error: can't create window";
        return false;
    }

    glfwSetKeyCallback(this->win, Window::global_key_event_callback);
    glfwSetCursorPosCallback(this->win, Window::global_cursor_pos_callback);
    glfwSetWindowSizeCallback(this->win, Window::global_window_size_callback);

    glfwMakeContextCurrent(this->win);
    gladLoadGL();
    glfwSwapInterval(1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Shader compilation is deferred
    for (Shader *shader : this->shaders) {
        shader->setup();
    }

    // Set our initial window size
    glfwGetWindowSize(this->win, &Window::width, &Window::height);

    return true;
}

void Window::draw() {
    // Clear the colorbuffer
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    //glViewport(0, 0, width, height);

    for (Shader *shader : this->shaders) {
        shader->use();
        shader->draw_layers();
    }

    glfwSwapBuffers(this->win);
}

void Window::process_events() {
    // This is where the window acts on the events sent from the engine
    std::optional<Event> event;
    while ((event = Window::engine.pop_outgoing_event()).has_value()) {
        if (event->type == WindowResizeRequest) {
            int width = std::get<int>(event->data[0]);
            int height = std::get<int>(event->data[1]);
            glfwSetWindowSize(this->win, width, height);
            glViewport(0, 0, width, height);

            PLOGI << "Got window resize request";
            PLOGI << "W: " << width << ", H: " << height;
        } else {
            PLOGE << "Got unknown event type";
        }
    }
}

void Window::main_loop() {
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    while (!glfwWindowShouldClose(this->win)) {
        Window::engine.process_events();
        this->process_events();

        auto start = high_resolution_clock::now();
        this->draw();
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(stop - start);
        auto fps = 1000000.0f / duration.count();

        if (fps_count++ < fps_samples) {
            fps_avg += fps;
        } else {
            PLOGD << fps_avg / fps_samples << " FPS";
            fps_count = 0;
            fps_avg = 0.0f;
        }

        glfwPollEvents();
    }

    for (Shader *shader : this->shaders) {
        // Begin tearing down GL resources
        shader->teardown();
    }

    glDeleteVertexArrays(1, &vao);
    glfwDestroyWindow(this->win);
    glfwTerminate();
}

void Window::add_layer(Layer *layer, Shader *shader) {
    // Layers are actually "stored" in the shader.
    // this makes rendering by shader easier, and
    // minimizes program switching.

    shader->add_layer(layer);

    // TODO: Can probably make this into an unordered_set or something faster
    if (std::find(this->shaders.begin(), this->shaders.end(), shader) == this->shaders.end()) {
        this->shaders.push_back(shader);
    }
}
