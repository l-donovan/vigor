#pragma once

#include "layer.h"

class ExampleLayer : public Layer {
    private:
        unsigned int VBO, VAO;
    public:
        ~ExampleLayer();

        void setup();
        void draw();
};
