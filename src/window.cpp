#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "vigor/layer.h"
#include "vigor/shader.h"
#include "vigor/window.h"

// `_ROOT_DIR` is set via cmake
//std::string ROOT_DIR(_ROOT_DIR);

using std::string;

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

int Window::width = 0;
int Window::height = 0;

void Window::global_cursor_pos_callback(GLFWwindow *window, double x_pos, double y_pos) {
    double x_adj = x_pos / Window::width;
    double y_adj = y_pos / Window::height;

    if (x_adj < 0.0 || x_adj > 1.0 || y_adj < 0.0 || y_adj > 1.0)
        return;

    Window::cursor_pos(x_adj, y_adj);
}

void Window::global_window_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);

    Window::width = width;
    Window::height = height;

    Window::window_size(width, height);
}

void Window::global_key_event_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else {
        Window::key_event(key, scancode, action, mods);
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
    std::cerr << "Error: " << description << std::endl;
}

bool Window::startup() {
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit()) {
        std::cerr << "Error: glfwInit" << std::endl;
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
        std::cerr << "Error: can't create window" << std::endl;
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

void Window::main_loop() {
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    while (!glfwWindowShouldClose(this->win)) {
        // TODO: this->logic();

        // Clear the colorbuffer
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(0, 0, width, height);

        for (Shader *shader : this->shaders) {
            shader->use();
            shader->draw_layers();
        }

        glfwSwapBuffers(this->win);
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
