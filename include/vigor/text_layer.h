#pragma once
#include <glm/glm.hpp>

#include "layer.h"
#include "text_renderer.h"

#include <string>
#include <vector>

using std::string;
using std::vector;

struct CharacterGeometry {
    float vertices[6][4];
    unsigned int texture_id;
    bool visible;
};

class TextLayer : public Layer {
    private:
        unsigned int VBO, VAO;
        unsigned int char_height = 48;
        float scale = 0.5f;
        float x, y;
        string text;
        vector<CharacterGeometry> geometry;
        TextRenderer renderer;
    public:
        ~TextLayer();

        void setup();
        void draw();
        void update();
        void recalculate_visibility();
        void recalculate_visibility_2();
        void set_text(string text);
        void set_position(float x, float y);
};
