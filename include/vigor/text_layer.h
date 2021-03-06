#pragma once
#include <glm/glm.hpp>

#include "text_buffer.h"
#include "layer.h"

#include <iostream>
#include <string>

using std::string;

struct Glyph {
    void *data;              // Texture data
    glm::ivec2   size;       // Size of glyph
    glm::ivec2   bearing;    // Offset from baseline to left/top of glyph
    unsigned int advance;    // Horizontal offset to advance to next glyph
    glm::vec2 uv_start;
    glm::vec2 uv_stop;
};

class TextLayer : public Layer {
    private:
        GLuint vbo_vertices = 0;
        GLuint vbo_colors = 0;
        GLuint vbo_uvs = 0;
        GLuint ibo_faces = 0;

        GLuint atlas_texture_id = 0;
        unsigned int atlas_width = 0;
        unsigned int atlas_height = 0;

        float x = 0.0f;
        float y = 0.0f;
        float last_y = 1.0f;
        int vertical_char_offset = 0;
        float scale = 0.5f;
        string text;

        unsigned int columns = 80;
        unsigned int rows = 24;
        unsigned int char_count = 80 * 24;
        int start_line = 0;
        int last_start_line = 0;
        float *vertices = nullptr;
        float *uvs = nullptr;
        float *colors = nullptr;
        GLushort *faces = nullptr;

        TextBuffer *buffer = nullptr;

        std::string font_path;
        int font_height = 0;

        int top_line_idx = 0;
        unsigned int bottom_line_idx = 0;
    public:
        TextLayer() {};

        void set_font(string font_path, int font_height);
        bool rasterize_font();
        void setup();
        void update();
        void draw();
        void teardown();
        void set_text(string text);
        void set_position(float x, float y);
        void calculate_dimensions();
        void allocate_attribute_buffers();
        void calculate_attribute_buffers();
        void bind_text_buffer(TextBuffer *buffer);

        void set_start_line(unsigned int line_num);
        unsigned int get_start_line();
};
