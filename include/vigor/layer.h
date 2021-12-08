#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

class Layer {
    public:
        int shader_id;

        virtual void setup() = 0;
        virtual void draw() = 0;
        virtual void update() = 0;
        virtual void teardown() = 0;
};
