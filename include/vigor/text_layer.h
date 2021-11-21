#pragma once
#include <glm/glm.hpp>

#include "layer.h"

#include <string>
#include <vector>

using std::string;
using std::vector;

struct Character {
    unsigned int texture_id; // ID handle of the glyph texture
    glm::ivec2   size;      // Size of glyph
    glm::ivec2   bearing;   // Offset from baseline to left/top of glyph
    unsigned int advance;   // Horizontal offset to advance to next glyph
};

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
