#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <plog/Log.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "vigor/global.h"
#include "vigor/engine.h"
#include "vigor/event.h"
#include "vigor/layer.h"
#include "vigor/shader.h"
#include "vigor/window.h"

using std::string;

Window::Window(string window_title, int initial_width, int initial_height) {
    this->window_title = window_title;
    this->initial_width = initial_width;
    this->initial_height = initial_height;

    this->single_frame_duration = std::chrono::duration<double, std::milli>(1000.0f / this->fps_target);
}

Window::~Window() {
}

Engine* Window::engine = nullptr;

int Window::width = 0;
int Window::height = 0;

int frame_count = 0;
float frame_duration_sum = 0.0f;
int fps_samples = 30;

void Window::attach(Engine *engine) {
    Window::engine = engine;
}

void Window::global_cursor_pos_callback(GLFWwindow *window, double x_pos, double y_pos) {
    double x_adj = x_pos / Window::width;
    double y_adj = y_pos / Window::height;

    if (x_adj < 0.0 || x_adj > 1.0 || y_adj < 0.0 || y_adj > 1.0)
        return;

    Window::engine->add_incoming_event({CursorPosition, {x_adj, y_adj}});
}

void Window::global_window_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);

    Window::width = width;
    Window::height = height;

    Window::engine->add_incoming_event({WindowResize, {width, height}});
}

void Window::global_key_event_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else {
        Window::engine->add_incoming_event({Key, {key, scancode, action, mods}});
    }
}

static void glfw_error_callback(int error, const char *description) {
    PLOGE << "Error: " << description;
}

bool Window::startup() {
    // First we want to check if the attached Engine needs us to do anything.
    // Any GL/GLFW-specific events will absolutely break things, however, since
    // neither are initialized at this point.
    this->process_events();

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

    int width, height;

    while ((event = Window::engine->pop_outgoing_event()).has_value()) {
        switch (event->type) {
        case WindowResizeRequest:
            width = std::get<int>(event->data[0]);
            height = std::get<int>(event->data[1]);

            glfwSetWindowSize(this->win, width, height);
            glViewport(0, 0, width, height);

            PLOGI << "Got window resize request";
            PLOGI << "W: " << width << ", H: " << height;
            break;
        case LayerUpdateRequest:
            for (Shader *shader : this->shaders) {
                shader->update();
            }
            break;
        case LayerModifyRequest:
            PLOGD << "Got layer modify request";
            if (std::get<int>(event->data[0]) == EVENT_LAYER_ADD) {
                this->add_layer(std::get<Layer*>(event->data[1]), std::get<Shader*>(event->data[2]));
            }
            break;
        default:
            PLOGE << "Got unknown event type";
            break;
        }
    }
}

void Window::main_loop() {
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    auto start = std::chrono::high_resolution_clock::now();
    auto stop = std::chrono::high_resolution_clock::now();

    std::chrono::duration<float, std::milli> sleep_duration;
    bool ahead;

    while (!glfwWindowShouldClose(this->win)) {
        // Start the timer where we left off
        start = stop;

        // Process our events, and tell the engine to process its events
        Window::engine->process_events();
        this->process_events();

        // Draw frame and time the draw call
        this->draw();

        // Poll for any new glfw events
        glfwPollEvents();

        // Calculate the time it took to draw the current frame, and determine if we are ahead
        // of or behind schedule
        stop = std::chrono::high_resolution_clock::now();
        this->current_frame_duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        sleep_duration = this->single_frame_duration - this->current_frame_duration;
        ahead = sleep_duration.count() > 0;

        if (frame_count++ < fps_samples) {
            frame_duration_sum += this->current_frame_duration.count();
        } else {
            if (ahead) {
                // We're ahead of schedule! (Or at least on time)
                float ahead_percent = 100.0f * sleep_duration.count() / (1000.0f / this->fps_target);
                PLOGD << "FPS: " << this->fps_target << ", ahead " << ahead_percent << "%";
            } else {
                // We're behind schedule!
                auto fps = 1000.0f / (frame_duration_sum / fps_samples);
                float behind_percent = -100.0f * sleep_duration.count() / (1000.0f / this->fps_target);
                PLOGW << "FPS: " << fps << ", behind " << behind_percent << "%";
            }

            frame_count = 0;
            frame_duration_sum = 0.0f;
        }

        // Only sleep if we're ahead of schedule
        if (ahead) {
            std::this_thread::sleep_for(sleep_duration);
        }

        // Reset the clock after any sleeping overhead or actual sleeping
        stop = std::chrono::high_resolution_clock::now();
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
