#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <string>
#include <vector>

#include "layer.h"
#include "shader.h"

using std::string;
using std::vector;

class Window {
    private:
        string window_title;
        int initial_width;
        int initial_height;
        GLFWwindow *win;
        vector<Shader*> shaders;

        void on_resize(int width, int height);
        void (*logic)();

        static void (*cursor_pos)(double x_pos, double y_pos);
        static void (*window_size)(int width, int height);
        static void global_cursor_pos_callback(GLFWwindow* window, double x_pos, double y_pos);
        static void global_window_size_callback(GLFWwindow* window, int width, int height);
    public:
        Window(string window_title, int initial_width, int initial_height);
        ~Window();

        bool startup();
        void main_loop();
        void register_cursor_pos_fn(void (*fp)(double x_pos, double y_pos));
        void register_window_size_fn(void (*fp)(int width, int height));
        void add_layer(Layer *layer, Shader *shader);

        static int width;
        static int height;
};
