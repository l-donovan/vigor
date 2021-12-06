#pragma once
#include <glm/glm.hpp>

#include "layer.h"

#include <string>
#include <vector>

using std::string;
using std::vector;

struct Character {
    void *data;              // Texture data
    glm::ivec2   size;       // Size of glyph
    glm::ivec2   bearing;    // Offset from baseline to left/top of glyph
    unsigned int advance;    // Horizontal offset to advance to next glyph
    glm::vec2 uv_start;
    glm::vec2 uv_stop;
};

class TextLayer : public Layer {
    private:
        GLuint vbo_vertices, vbo_colors, vbo_uvs, ibo_faces;
        GLuint atlas_texture_id;
        float scale = 0.5f;
        float x, y;
        string text;
        vector<glm::vec4> vertices;
        vector<glm::vec2> uvs;
        vector<glm::vec4> colors;
        vector<GLushort> faces;
    public:
        ~TextLayer();

        std::string font_path;
        int font_height;

        void set_font(string font_path, int font_height);
        void setup();
        void draw();
        void update();
        bool load();
        void recalculate_visibility();
        void set_text(string text);
        void set_position(float x, float y);
};
