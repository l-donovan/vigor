#pragma once

#include "layer.h"

#include <string>

using std::string;

class TextLayer : public Layer {
    private:
        unsigned int VBO, VAO;
        unsigned int char_height = 48;
        float x, y;
        string text;
    public:
        ~TextLayer();

        void setup();
        void draw();
        void set_text(string text);
        void set_char(int row, int col, char ch);
        void set_position(float x, float y);
};
