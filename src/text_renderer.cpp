// GL imports
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

// Local imports
#include "vigor/text_renderer.h"

// General imports
#include <iostream>
#include <map>
#include <string>

using std::string;

TextRenderer::TextRenderer() {
}

TextRenderer::~TextRenderer() {
}

void TextRenderer::set_font(string font_path, int font_height) {
    this->font_path = font_path;
    this->font_height = font_height;
}

bool TextRenderer::load() {
    FT_Library ft;
    std::map<GLchar, Character> characters;

    if (FT_Init_FreeType(&ft)) {
        std::cerr << "Could not initialize FreeType library" << std::endl;
        return false;
    }

    if (this->font_path.empty()) {
        std::cerr << "Font path is unset" << std::endl;
        return false;
    }

    FT_Face face;
    if (FT_New_Face(ft, this->font_path.c_str(), 0, &face)) {
        std::cerr << "Failed to load font" << std::endl;
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, this->font_height);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    unsigned int total_width = 0;

    // Load ASCII charset
    for (unsigned char c = 0; c < 128; ++c) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "Failed to load glyph #" << static_cast<unsigned int>(c) << std::endl;
            continue;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };

        characters.insert(std::pair<char, Character>(c, character));

        total_width += character.size.x;
    }

    unsigned int texture_width = total_width % 256;
    unsigned int texture_height = total_width / 256;

    std::cout << "Width: " << texture_width << "px, Height: " << texture_height << "px" << std::endl;

    GLuint atlas_texture_id;
    glGenTextures(1, &atlas_texture_id);
    glBindTexture(GL_TEXTURE_2D, atlas_texture_id);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        total_width,
        total_width,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        NULL
    );

    // TODO: Convert to atlas and discard textures
    for (auto const& [key, val] : characters) {
        std::cout << val.texture_id << std::endl;
        std::cout << glCopyImageSubData << std::endl;
        glCopyImageSubData(
            val.texture_id,   // Source texture
            GL_TEXTURE_2D,    // Source type
            0,                // Source mipmap level
            0,                // Source X
            0,                // Source Y
            0,                // Source Z
            atlas_texture_id, // Destination texture
            GL_TEXTURE_2D,    // Destination type
            0,                // Destination mipmap level
            0,                // Destination X
            0,                // Destination Y
            0,                // Destination Z
            val.size.x,       // Source width
            val.size.y,       // Source height
            1                 // Source depth
        );
    }

    // Discard freetype objects
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // Configure VAO/VBO for texture quads
    glGenVertexArrays(1, &this->VAO);
    glGenBuffers(1, &this->VBO);
    glBindVertexArray(this->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

void TextRenderer::draw() {
}
