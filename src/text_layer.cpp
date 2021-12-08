#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include <freetype/ftlcdfil.h>
#include FT_FREETYPE_H

#include "vigor/global.h"
#include "vigor/text_buffer.h"
#include "vigor/text_layer.h"
#include "vigor/window.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#define VEC2(TARGET, INDEX, V1, V2) {\
    TARGET[2 * (INDEX)    ] = V1;\
    TARGET[2 * (INDEX) + 1] = V2;\
}

#define VEC4(TARGET, INDEX, V1, V2, V3, V4) {\
    TARGET[4 * (INDEX)    ] = V1;\
    TARGET[4 * (INDEX) + 1] = V2;\
    TARGET[4 * (INDEX) + 2] = V3;\
    TARGET[4 * (INDEX) + 3] = V4;\
}

using std::string;

std::map<GLchar, Character> characters;

void TextLayer::setup() {
    glGenBuffers(1, &this->vbo_vertices);
    glGenBuffers(1, &this->vbo_uvs);
    glGenBuffers(1, &this->vbo_colors);
    glGenBuffers(1, &this->ibo_faces);

    // Set up all the vectors
    unsigned int columns = 80;
    unsigned int rows = 24;

    unsigned int char_count = columns * rows;

    this->vertices = (float*) malloc(2 * sizeof(float) * 4 * char_count);
    this->uvs = (float*) malloc(2 * sizeof(float) * 4 * char_count);
    this->colors = (float*) malloc(4 * sizeof(float) * 4 * char_count);
    this->faces = (GLushort*) malloc(sizeof(GLushort) * 6 * char_count);

    for (unsigned int i = 0; i < char_count; ++i) {
        // Face 1
        this->faces[6 * i    ] = 4 * (i + 1) - 4;
        this->faces[6 * i + 1] = 4 * (i + 1) - 3;
        this->faces[6 * i + 2] = 4 * (i + 1) - 2;

        // Face 2
        this->faces[6 * i + 3] = 4 * (i + 1) - 4;
        this->faces[6 * i + 4] = 4 * (i + 1) - 2;
        this->faces[6 * i + 5] = 4 * (i + 1) - 1;
    }
}

void TextLayer::set_font(string font_path, int font_height) {
    this->font_path = font_path;
    this->font_height = font_height;
    this->load();
}

void TextLayer::bind_buffer(TextBuffer* buffer) {
    this->buffer = buffer;
}

bool TextLayer::load() {
    FT_Library ft;

    if (FT_Init_FreeType(&ft)) {
        PLOGE << "Could not initialize FreeType library";
        return false;
    }

    if (this->font_path.empty()) {
        PLOGE << "Font path is unset";
        return false;
    }

    FT_Face face;
    if (FT_New_Face(ft, this->font_path.c_str(), 0, &face)) {
        PLOGE << "Failed to load font";
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, this->font_height);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    unsigned int char_count = 128;
    unsigned char *bitmap_data;
    Character character;

    unsigned int current_width = 0;
    atlas_width = 512;
    atlas_height = 0;

    // Clear existing characters
    characters.clear();

    // Load ASCII charset
    for (unsigned char c = 0; c < char_count; ++c) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            PLOGE << "Failed to load glyph #" << static_cast<unsigned int>(c);
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
            current_width = character.size.x;
            atlas_height += this->font_height;
        }
    }

    atlas_height += this->font_height;

    PLOGI << "Atlas width: " << atlas_width << "px, height: " << atlas_height << "px";

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

        PLOGD
            << "Character #" << static_cast<unsigned int>(key)
            << ": UV1 (" << uv_x1 << ", " << uv_y1 << ")"
            << ", UV2 (" << uv_x2 << ", " << uv_y2 << ")";

        // We don't need the bitmap data anymore now that it's in a texture atlas
        free(ch.data);

        x += ch.size.x;
    }

    // Discard freetype objects
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    return true;
}

void TextLayer::teardown() {
    glDeleteBuffers(1, &this->vbo_vertices);
    glDeleteBuffers(1, &this->vbo_uvs);
    glDeleteBuffers(1, &this->vbo_colors);
    glDeleteBuffers(1, &this->ibo_faces);

    glDeleteTextures(1, &this->atlas_texture_id);
}

void TextLayer::set_text(string text) {
    this->text = text;
    this->update();
    PLOGD
        << "C: " << this->text.size()
        << ", V: " << 4 * this->text.size()
        << ", F: " << 2 * this->text.size();
}

void TextLayer::set_position(float x, float y) {
    this->x = x * Window::width;
    this->y = (1.0f - y) * Window::height - this->scale * this->font_height;
}

void TextLayer::update() {
    PLOGD << "Update called";

    float bearing_x, bearing_y, width, height, advance, x_pos, y_pos;
    float last_x = -1.0f;
    float last_y = 1.0f;
    float to_screen_width = 2.0f / Window::width;
    float to_screen_height = 2.0f / Window::height;
    float space_advance = characters[' '].advance / 64.0f * to_screen_width;
    float font_height = this->font_height * to_screen_height;

    // Set up all the vectors
    unsigned int columns = 80;
    unsigned int rows = 24;

    char_count = std::min(columns * rows, (unsigned int)this->text.length());

    unsigned int idx = 0;
    for (unsigned int i = 0; i < char_count; ++i) {
        char c = this->text[i];
        Character ch = characters[c];

        // Handle geometry correctly for whitespace characters
        if (c == '\n') {
            last_y -= font_height;
            last_x = -1.0f;
            continue;
        } else if (c == '\r') {
            last_x = -1.0f;
            continue;
        } else if (c == ' ') {
            last_x += space_advance;
            continue;
        } else if (c == '\t') {
            last_x += 4.0f * space_advance;
            continue;
        }

        advance = ch.advance / 64.0f * to_screen_width;
        bearing_x = float(ch.bearing.x) * to_screen_width;
        bearing_y = float(ch.bearing.y) * to_screen_height;
        width = float(ch.size.x) * to_screen_width;
        height = float(ch.size.y) * to_screen_height;

        x_pos = last_x + bearing_x;
        y_pos = last_y - (font_height - bearing_y);

        VEC2(vertices, idx + 0, x_pos,         y_pos - height)
        VEC2(vertices, idx + 1, x_pos,         y_pos)
        VEC2(vertices, idx + 2, x_pos + width, y_pos)
        VEC2(vertices, idx + 3, x_pos + width, y_pos - height)

        VEC2(uvs, idx + 0, ch.uv_start.x, ch.uv_stop.y)
        VEC2(uvs, idx + 1, ch.uv_start.x, ch.uv_start.y)
        VEC2(uvs, idx + 2, ch.uv_stop.x,  ch.uv_start.y)
        VEC2(uvs, idx + 3, ch.uv_stop.x,  ch.uv_stop.y)

        VEC4(colors, idx + 0, 1.0f, 1.0f, 1.0f, 1.0f)
        VEC4(colors, idx + 1, 1.0f, 1.0f, 1.0f, 1.0f)
        VEC4(colors, idx + 2, 1.0f, 1.0f, 1.0f, 1.0f)
        VEC4(colors, idx + 3, 1.0f, 1.0f, 1.0f, 1.0f)

        last_x += advance;
        idx += 4;
    }

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_vertices);
    glBufferData(GL_ARRAY_BUFFER, 4 * 2 * char_count * sizeof(float), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_uvs);
    glBufferData(GL_ARRAY_BUFFER, 4 * 2 * char_count * sizeof(float), uvs, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_colors);
    glBufferData(GL_ARRAY_BUFFER, 4 * 4 * char_count * sizeof(float), colors, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ibo_faces);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * char_count * sizeof(GLushort), faces, GL_STATIC_DRAW);
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
    glVertexAttribPointer(vertex_position, 2, GL_FLOAT, GL_FALSE, 0, 0);

    GLint uv_position = glGetAttribLocation(this->shader_id, "tex_coord");
    glEnableVertexAttribArray(uv_position);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_uvs);
    glVertexAttribPointer(uv_position, 2, GL_FLOAT, GL_FALSE, 0, 0);

    GLint color_position = glGetAttribLocation(this->shader_id, "color");
    glEnableVertexAttribArray(color_position);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_colors);
    glVertexAttribPointer(color_position, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ibo_faces);
    glDrawElements(GL_TRIANGLES, 6 * char_count, GL_UNSIGNED_SHORT, 0);

    glDisableVertexAttribArray(vertex_position);
    glDisableVertexAttribArray(uv_position);
    glDisableVertexAttribArray(color_position);
}
