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
#include "vigor/window.h"

// General imports
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using std::string;
using std::vector;

TextRenderer::TextRenderer() {
    this->vertices.push_back(glm::vec4(-1.0, -1.0f, 0.0f, 1.0f));
    this->vertices.push_back(glm::vec4(1.0f, -1.0f, 0.0f, 1.0f));
    this->vertices.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));

    this->uvs.push_back(glm::vec2(0.615836f, 1.0f));
    this->uvs.push_back(glm::vec2(0.615836f, 1.0f));
    this->uvs.push_back(glm::vec2(0.615836f, 1.0f));

    this->colors.push_back(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    this->colors.push_back(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    this->colors.push_back(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

    this->faces.push_back(0);
    this->faces.push_back(1);
    this->faces.push_back(2);
}

TextRenderer::~TextRenderer() {
    glDeleteBuffers(1, &this->vbo_mesh_vertices);
    glDeleteBuffers(1, &this->vbo_mesh_uvs);
    glDeleteBuffers(1, &this->vbo_mesh_colors);
    glDeleteBuffers(1, &this->ibo_mesh_faces);
    glDeleteTextures(1, &this->atlas_texture_id);
}

void TextRenderer::set_font(string font_path, int font_height) {
    this->font_path = font_path;
    this->font_height = font_height;
}

bool TextRenderer::load() {
    glGenBuffers(1, &this->vbo_mesh_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_mesh_vertices);
    glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(glm::vec4), this->vertices.data(), GL_DYNAMIC_DRAW);

    glGenBuffers(1, &this->vbo_mesh_uvs);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_mesh_uvs);
    glBufferData(GL_ARRAY_BUFFER, this->uvs.size() * sizeof(glm::vec2), this->uvs.data(), GL_DYNAMIC_DRAW);

    glGenBuffers(1, &this->vbo_mesh_colors);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_mesh_colors);
    glBufferData(GL_ARRAY_BUFFER, this->colors.size() * sizeof(glm::vec4), this->colors.data(), GL_DYNAMIC_DRAW);

    glGenBuffers(1, &this->ibo_mesh_faces);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ibo_mesh_faces);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->faces.size() * sizeof(GLushort), this->faces.data(), GL_DYNAMIC_DRAW);

    attribute_vertex = glGetAttribLocation(this->shader_id, "vertex");
    if (attribute_vertex == -1) {
        std::cerr << "Could not bind vertex attribute" << std::endl;
        return false;
    }

    attribute_uv = glGetAttribLocation(this->shader_id, "tex_coord");
    if (attribute_uv == -1) {
        std::cerr << "Could not bind UV attribute" << std::endl;
        return false;
    }

    attribute_color = glGetAttribLocation(this->shader_id, "color");
    if (attribute_color == -1) {
        std::cerr << "Could not bind color attribute" << std::endl;
        return false;
    }

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

    unsigned int char_count = 128;
    GLuint *char_textures = (GLuint*) malloc(char_count * sizeof(GLuint));

    unsigned int current_width = 0;
    unsigned int atlas_width = 0;
    unsigned int atlas_height = 0;
    unsigned int total_height = 0;
    unsigned int limit_width = 1024;

    // Load ASCII charset
    for (unsigned char c = 0; c < char_count; ++c) {
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

        char_textures[c] = texture;
        characters.insert(std::pair<char, Character>(c, character));

        current_width += character.size.x;

        if (current_width > limit_width) {
            if (current_width - character.size.x > atlas_width)
                atlas_width = current_width - character.size.x;
            current_width = character.size.x;
            atlas_height += this->font_height;
        }
    }

    std::cout << "Atlas width: " << atlas_width << "px, height: " << atlas_height << "px" << std::endl;

    // Create atlas
    glGenTextures(1, &this->atlas_texture_id);
    glBindTexture(GL_TEXTURE_2D, this->atlas_texture_id);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        atlas_width,
        atlas_height,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        NULL
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Populate atlas
    int x = 0;
    int y = 0;
    for (auto const& [key, ch] : characters) {
        if (x + ch.size.x > atlas_width) {
            x = 0;
            y += this->font_height;
        }

        glCopyImageSubData(
            ch.texture_id,          // Source texture
            GL_TEXTURE_2D,          // Source type
            0,                      // Source mipmap level
            0,                      // Source X
            0,                      // Source Y
            0,                      // Source Z
            this->atlas_texture_id, // Destination texture
            GL_TEXTURE_2D,          // Destination type
            0,                      // Destination mipmap level
            x,                      // Destination X
            y,                      // Destination Y
            0,                      // Destination Z
            ch.size.x,              // Source width
            ch.size.y,              // Source height
            0                       // Source depth
        );

        float uv_x = float(x) / atlas_width;
        float uv_y = float(y) / atlas_height;

        std::cout << "Character #" << static_cast<unsigned int>(key) << " UV (" << uv_x << ", " << uv_y << ")" << std::endl;

        x += ch.size.x;
    }

    // Discard individual character textures
    glDeleteTextures(char_count, char_textures);

    // Discard freetype objects
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    return true;
}

void TextRenderer::set_shader(GLuint shader_id) {
    this->shader_id = shader_id;
}

void TextRenderer::put_char(char ch, int x, int y) {

};

void TextRenderer::draw() {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection = glm::ortho(
        0.0f,
        static_cast<float>(Window::width),
        0.0f,
        static_cast<float>(Window::height)
    );

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(this->shader_id, "texture"), 0);
    glBindTexture(GL_TEXTURE_2D, this->atlas_texture_id);

    //glUniformMatrix4fv(glGetUniformLocation(this->shader_id, "model"), 1, 0, glm::value_ptr(model));
    //glUniformMatrix4fv(glGetUniformLocation(this->shader_id, "view"), 1, 0, glm::value_ptr(view));
    //glUniformMatrix4fv(glGetUniformLocation(this->shader_id, "projection"), 1, 0, glm::value_ptr(projection));

    GLint vertex_position = glGetAttribLocation(this->shader_id, "vertex");
    glEnableVertexAttribArray(vertex_position);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_mesh_vertices);
    glVertexAttribPointer(vertex_position, 4, GL_FLOAT, GL_FALSE, 0, 0);

    GLint uv_position = glGetAttribLocation(this->shader_id, "tex_coord");
    glEnableVertexAttribArray(uv_position);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_mesh_colors);
    glVertexAttribPointer(uv_position, 2, GL_FLOAT, GL_FALSE, 0, 0);

    GLint color_position = glGetAttribLocation(this->shader_id, "color");
    glEnableVertexAttribArray(color_position);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_mesh_colors);
    glVertexAttribPointer(color_position, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ibo_mesh_faces);
    glDrawElements(GL_TRIANGLES, this->faces.size(), GL_UNSIGNED_SHORT, 0);

    glDisableVertexAttribArray(attribute_vertex);
    glDisableVertexAttribArray(attribute_uv);
    glDisableVertexAttribArray(attribute_color);
}
