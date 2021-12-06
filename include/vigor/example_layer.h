#pragma once

#include "layer.h"

class ExampleLayer : public Layer {
    private:
        unsigned int VBO, VAO;
        GLuint vbo_vertices, vbo_colors, ibo_faces;
    public:
        void setup();
        void draw();
        void teardown();
};
