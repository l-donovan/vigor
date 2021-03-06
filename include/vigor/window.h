#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <chrono>
#include <string>
#include <vector>

#include "engine.h"
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

        int fps_target = 60;
        std::chrono::duration<float, std::milli> single_frame_duration;
        std::chrono::duration<float, std::milli> current_frame_duration;

        void process_events();
        void draw();

        static Engine *engine;

        static void global_cursor_pos_callback(GLFWwindow *window, double x_pos, double y_pos);
        static void global_window_size_callback(GLFWwindow *window, int width, int height);
        static void global_key_event_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
    public:
        Window(string window_title, int initial_width, int initial_height);
        ~Window();

        bool startup();
        void main_loop();
        void add_layer(Layer *layer, Shader *shader);
        void attach(Engine *engine);

        static int width;
        static int height;
};
