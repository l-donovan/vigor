#pragma once
#include <glm/glm.hpp>

#include "text_buffer.h"
#include "layer.h"

#include <iostream>
#include <string>
#include <vector>

using std::string;
using std::vector;

struct Character {
    unsigned int texture_id; // ID handle of the glyph texture
    glm::ivec2   size;       // Size of glyph
    glm::ivec2   bearing;    // Offset from baseline to left/top of glyph
    unsigned int advance;    // Horizontal offset to advance to next glyph
    glm::vec2 uv_start;
    glm::vec2 uv_stop;
};

struct CharacterGeometry {
    float vertices[6][4];
    unsigned int texture_id;
    bool visible;
};

class TextLayer : public Layer {
    private:
        GLuint vbo_vertices = 0,
            vbo_colors = 0,
            vbo_uvs = 0,
            ibo_faces = 0;
        GLuint atlas_texture_id = 0;
        float scale = 0.5f;
        float x = 0,
            y = 0;
        string text;
        vector<glm::vec4> vertices;
        vector<glm::vec2> uvs;
        vector<glm::vec4> colors;
        vector<GLushort> faces;
        TextBuffer *buffer = nullptr;

        std::string font_path;
        int font_height = 0;
    public:
        TextLayer() {};
        ~TextLayer();

        void set_font(string font_path, int font_height);
        void setup();
        void draw();
        void update();
        bool load();
        void set_text(string text);
        void set_position(float x, float y);
        void bind_buffer(TextBuffer *buffer);
};
