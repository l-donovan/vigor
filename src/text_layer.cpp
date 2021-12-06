#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include <freetype/ftlcdfil.h>
#include FT_FREETYPE_H

#include "vigor/text_layer.h"
#include "vigor/window.h"

#include <cmath>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// TODO: Remove after testing
#include <chrono>
using namespace std::chrono;

using std::string;

std::map<GLchar, Character> characters;

void TextLayer::setup() {
    glGenBuffers(1, &this->vbo_vertices);
    glGenBuffers(1, &this->vbo_uvs);
    glGenBuffers(1, &this->vbo_colors);
    glGenBuffers(1, &this->ibo_faces);
}

void TextLayer::set_font(string font_path, int font_height) {
    this->font_path = font_path;
    this->font_height = font_height;
    this->load();
}

bool TextLayer::load() {
    FT_Library ft;

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
    unsigned char *bitmap_data;
    Character character;

    unsigned int current_width = 0;
    unsigned int atlas_width = 512;
    unsigned int atlas_height = 0;

    // Load ASCII charset
    for (unsigned char c = 0; c < char_count; ++c) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "Failed to load glyph #" << static_cast<unsigned int>(c) << std::endl;
            continue;
        }

        if (!FT_Get_Char_Index(face, c)) {
            // Now store character for later use
            character = {
                nullptr,
                glm::ivec2(0),
                glm::ivec2(0),
                static_cast<unsigned int>(0)
            };
        } else {
            if (FT_Error err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_LCD)) {
                std::cerr << "Glyph rendering error: " << err << std::endl;
            }

            // Copy bitmap data to some non-ephemeral location since we won't be writing
            // it to a texture right away
            size_t bitmap_size = face->glyph->bitmap.rows * face->glyph->bitmap.pitch;

            // The pitch is given in Bytes, thus bitmap_size is already in Bytes
            bitmap_data = (unsigned char*) malloc(bitmap_size);
            memcpy(bitmap_data, face->glyph->bitmap.buffer, bitmap_size);

            // Now store character for later use
            character = {
                bitmap_data,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x)
            };
        }

        characters.insert(std::pair<char, Character>(c, character));

        current_width += character.size.x;

        if (current_width > atlas_width) {
            if (current_width - character.size.x > atlas_width)
                atlas_width = current_width - character.size.x;
            current_width = character.size.x;
            atlas_height += this->font_height;
        }
    }

    atlas_height += this->font_height;

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

        // Write the character's bitmap to the atlas
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            x,
            y,
            ch.size.x,
            ch.size.y,
            GL_RED,
            GL_UNSIGNED_BYTE,
            ch.data
        );

        // Calculate the character's UV position in the atlas
        float uv_x1 = float(x) / atlas_width;
        float uv_y1 = float(y) / atlas_height;
        float uv_x2 = float(x + ch.size.x) / atlas_width;
        float uv_y2 = float(y + ch.size.y) / atlas_height;

        characters[key].uv_start = glm::vec2(uv_x1, uv_y1);
        characters[key].uv_stop = glm::vec2(uv_x2, uv_y2);

        std::cout
            << "Character #" << static_cast<unsigned int>(key) << std::endl
            << "  UV1 (" << uv_x1 << ", " << uv_y1 << ")" << std::endl
            << "  UV2 (" << uv_x2 << ", " << uv_y2 << ")" << std::endl;

        // We don't need the bitmap data anymore now that it's in a texture atlas
        free(ch.data);

        x += ch.size.x;
    }

    // Discard freetype objects
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    return true;
}

TextLayer::~TextLayer() {
    glDeleteBuffers(1, &this->vbo_vertices);
    glDeleteBuffers(1, &this->vbo_uvs);
    glDeleteBuffers(1, &this->vbo_colors);
    glDeleteBuffers(1, &this->ibo_faces);

    glDeleteTextures(1, &this->atlas_texture_id);
}

void TextLayer::set_text(string text) {
    this->text = text;
    this->update();
}

void TextLayer::set_position(float x, float y) {
    this->x = x * Window::width;
    this->y = (1.0f - y) * Window::height - this->scale * this->font_height;
    this->recalculate_visibility();
}

void TextLayer::recalculate_visibility() {
}

void TextLayer::update() {
    float last_x = -1.0f;

    vertices.clear();
    uvs.clear();
    colors.clear();
    faces.clear();

    // NOTE: This chunk of commented code is useful for ensuring the texture atlas is being calculated correctly

    //vertices.push_back(glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f));
    //vertices.push_back(glm::vec4(-1.0f,  1.0f, 0.0f, 1.0f));
    //vertices.push_back(glm::vec4( 1.0f,  1.0f, 0.0f, 1.0f));
    //vertices.push_back(glm::vec4( 1.0f, -1.0f, 0.0f, 1.0f));

    //uvs.push_back(glm::vec2(0.0f, 1.0f));
    //uvs.push_back(glm::vec2(0.0f, 0.0f));
    //uvs.push_back(glm::vec2(1.0f, 0.0f));
    //uvs.push_back(glm::vec2(1.0f, 1.0f));

    //colors.push_back(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    //colors.push_back(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    //colors.push_back(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    //colors.push_back(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    //faces.push_back(vertices.size() - 4);
    //faces.push_back(vertices.size() - 3);
    //faces.push_back(vertices.size() - 2);
    //faces.push_back(vertices.size() - 4);
    //faces.push_back(vertices.size() - 2);
    //faces.push_back(vertices.size() - 1);

    for (unsigned char c : this->text) {
        Character ch = characters[c];

        float bearing_x = 2.0f * ch.bearing.x / Window::width;
        float width = 2.0f * ch.size.x / Window::width;
        float height = 2.0f * ch.size.y / Window::height;
        float advance = 2.0f * ch.advance / 64.0f / Window::width;

        float x_pos = last_x + bearing_x;
        float y_pos = 1.0f - 2.0f * (float)(this->font_height - ch.bearing.y) / Window::height;

        vertices.push_back(glm::vec4(x_pos,         y_pos - height, 0.0f, 1.0f));
        vertices.push_back(glm::vec4(x_pos,         y_pos,          0.0f, 1.0f));
        vertices.push_back(glm::vec4(x_pos + width, y_pos,          0.0f, 1.0f));
        vertices.push_back(glm::vec4(x_pos + width, y_pos - height, 0.0f, 1.0f));

        glm::vec2 uv_start = ch.uv_start;
        glm::vec2 uv_stop = ch.uv_stop;

        uvs.push_back(glm::vec2(uv_start.x, uv_stop.y));
        uvs.push_back(glm::vec2(uv_start.x, uv_start.y));
        uvs.push_back(glm::vec2(uv_stop.x, uv_start.y));
        uvs.push_back(glm::vec2(uv_stop.x, uv_stop.y));

        colors.push_back(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        colors.push_back(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        colors.push_back(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        colors.push_back(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        faces.push_back(vertices.size() - 4);
        faces.push_back(vertices.size() - 3);
        faces.push_back(vertices.size() - 2);
        faces.push_back(vertices.size() - 4);
        faces.push_back(vertices.size() - 2);
        faces.push_back(vertices.size() - 1);

        last_x += advance;
    }

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_vertices);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec4), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_uvs);
    glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), uvs.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_colors);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec4), colors.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ibo_faces);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces.size() * sizeof(GLushort), faces.data(), GL_STATIC_DRAW);
}

void TextLayer::draw() {
    glm::vec3 translation(
        2.0f * this->x / Window::width,
        -2.0f * (1.0f - this->y / Window::height),
        0.0f);
    glm::mat4 model(1.0f);
    model = glm::translate(model, translation);

    GLuint model_location = glGetUniformLocation(this->shader_id, "model");
    glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model));

    GLuint texture_location = glGetUniformLocation(this->shader_id, "atlas");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->atlas_texture_id);
    glUniform1i(texture_location, 0);

    GLint vertex_position = glGetAttribLocation(this->shader_id, "vertex");
    glEnableVertexAttribArray(vertex_position);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_vertices);
    glVertexAttribPointer(vertex_position, 4, GL_FLOAT, GL_FALSE, 0, 0);

    GLint uv_position = glGetAttribLocation(this->shader_id, "tex_coord");
    glEnableVertexAttribArray(uv_position);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_uvs);
    glVertexAttribPointer(uv_position, 2, GL_FLOAT, GL_FALSE, 0, 0);

    GLint color_position = glGetAttribLocation(this->shader_id, "color");
    glEnableVertexAttribArray(color_position);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_colors);
    glVertexAttribPointer(color_position, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ibo_faces);
    glDrawElements(GL_TRIANGLES, this->faces.size(), GL_UNSIGNED_SHORT, 0);

    glDisableVertexAttribArray(vertex_position);
    glDisableVertexAttribArray(uv_position);
    glDisableVertexAttribArray(color_position);
}
