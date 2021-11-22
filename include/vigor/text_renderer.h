#pragma once

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <string>

struct Character {
    unsigned int texture_id; // ID handle of the glyph texture
    glm::ivec2   size;      // Size of glyph
    glm::ivec2   bearing;   // Offset from baseline to left/top of glyph
    unsigned int advance;   // Horizontal offset to advance to next glyph
};

class TextRenderer {
    private:
        GLuint VBO, VAO;
    public:
        TextRenderer();
        ~TextRenderer();

        std::string font_path;
        int font_height;

        void set_font(std::string font_path, int font_height);
        bool load();
        void draw();
};
