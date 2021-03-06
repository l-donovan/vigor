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

#define VEC2(TARGET, INDEX, V1, V2) {\
    TARGET[2 * (INDEX)    ] = V1;\
    TARGET[2 * (INDEX) + 1] = V2;\
}

#define VEC3(TARGET, INDEX, V1, V2, V3) {\
    TARGET[3 * (INDEX)    ] = V1;\
    TARGET[3 * (INDEX) + 1] = V2;\
    TARGET[3 * (INDEX) + 2] = V3;\
}

#define VEC4(TARGET, INDEX, V1, V2, V3, V4) {\
    TARGET[4 * (INDEX)    ] = V1;\
    TARGET[4 * (INDEX) + 1] = V2;\
    TARGET[4 * (INDEX) + 2] = V3;\
    TARGET[4 * (INDEX) + 3] = V4;\
}

using std::string;

std::map<GLchar, Glyph> glyphs;

void TextLayer::setup() {
    glGenBuffers(1, &this->vbo_vertices);
    glGenBuffers(1, &this->vbo_uvs);
    glGenBuffers(1, &this->vbo_colors);
    glGenBuffers(1, &this->ibo_faces);

    // This odd line forces the layer to do a full redraw
    // the next time `calculate_attribute_buffers` is called.
    this->last_y = 1.0f;

    // FIX: Not sure about this magic number
    this->last_start_line = -this->rows + 2;

    this->bottom_line_idx = this->rows - 1;

    this->calculate_dimensions();
}

void TextLayer::update() {
    this->calculate_dimensions();
    this->calculate_attribute_buffers();
}

void TextLayer::set_font(string font_path, int font_height) {
    this->font_path = font_path;
    this->font_height = font_height;
    this->rasterize_font();
    this->calculate_attribute_buffers();
}

void TextLayer::bind_text_buffer(TextBuffer* buffer) {
    this->buffer = buffer;
}

bool TextLayer::rasterize_font() {
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

    unsigned char *bitmap_data;
    Glyph glyph;

    unsigned int current_width = 0;
    atlas_width = 512;
    atlas_height = 0;

    // Clear existing glyphs
    glyphs.clear();

    // Load ASCII charset
    for (unsigned char c = 0; c < 128; ++c) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            PLOGE << "Failed to load glyph #" << static_cast<unsigned int>(c);
            continue;
        }

        if (!FT_Get_Char_Index(face, c)) {
            // Now store character glyph for later use
            glyph = {
                nullptr,
                glm::ivec2(0),
                glm::ivec2(0),
                static_cast<unsigned int>(0)
            };
        } else {
            // NOTE: Not sure this does anything
            if (FT_Error err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_LCD)) {
                std::cerr << "Glyph rendering error: " << err << std::endl;
            }

            // The pitch is given in Bytes, thus bitmap_size is already in Bytes
            size_t bitmap_size = face->glyph->bitmap.rows * face->glyph->bitmap.pitch;
            bitmap_data = (unsigned char*) malloc(bitmap_size);

            // Copy bitmap data to some non-ephemeral location since we won't be writing
            // it to a texture right away
            memcpy(bitmap_data, face->glyph->bitmap.buffer, bitmap_size);

            // Now store character glyph for later use
            glyph = {
                bitmap_data,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x)
            };
        }

        glyphs.insert(std::pair<char, Glyph>(c, glyph));

        current_width += glyph.size.x;

        if (current_width > atlas_width) {
            current_width = glyph.size.x;
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
    for (auto const& [key, glyph] : glyphs) {
        if (x + glyph.size.x > atlas_width) {
            x = 0;
            y += this->font_height;
        }

        if (!glyph.data) {
            continue;
        }

        // Write the character's bitmap to the atlas
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            x,
            y,
            glyph.size.x,
            glyph.size.y,
            GL_RED,
            GL_UNSIGNED_BYTE,
            glyph.data
        );

        // Calculate the character's UV position in the atlas
        float uv_x1 = float(x) / atlas_width;
        float uv_y1 = float(y) / atlas_height;
        float uv_x2 = float(x + glyph.size.x) / atlas_width;
        float uv_y2 = float(y + glyph.size.y) / atlas_height;

        glyphs[key].uv_start = glm::vec2(uv_x1, uv_y1);
        glyphs[key].uv_stop = glm::vec2(uv_x2, uv_y2);

        PLOGD
            << "Character #" << static_cast<unsigned int>(key)
            << ": UV1 (" << uv_x1 << ", " << uv_y1 << ")"
            << ", UV2 (" << uv_x2 << ", " << uv_y2 << ")";

        // We don't need the bitmap data anymore now that it's in a texture atlas
        free(glyph.data);

        x += glyph.size.x;
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
    this->calculate_attribute_buffers();

    PLOGD
        << "C: " << this->text.size()
        << ", V: " << 4 * this->text.size()
        << ", F: " << 2 * this->text.size();
}

void TextLayer::set_position(float x, float y) {
    this->x = x * Window::width;
    this->y = (1.0f - y) * Window::height;
}

void TextLayer::calculate_dimensions() {
    if (glyphs.contains(' ')) {
        float space_advance = glyphs[' '].advance / 64.0f;
        this->columns = ceil(1.0f * Window::width / space_advance);
        this->rows = ceil(1.0f * Window::height / this->font_height);
        PLOGD << "C: " << this->columns << " R: " << this->rows;
    } else {
        this->columns = 80;
        this->rows = 24;
    }

    this->char_count = this->columns * this->rows;
    this->allocate_attribute_buffers();
}

void TextLayer::allocate_attribute_buffers() {
    this->vertices = (float*) realloc(this->vertices, 2 * sizeof(float) * 4 * this->char_count);
    if (this->vertices == nullptr) {
        PLOGF << "Failed to allocate memory for vertices";
        return;
    }

    this->uvs = (float*) realloc(this->uvs, 2 * sizeof(float) * 4 * this->char_count);
    if (this->uvs == nullptr) {
        PLOGF << "Failed to allocate memory for UVs";
        return;
    }

    this->colors = (float*) realloc(this->colors, 4 * sizeof(float) * 4 * this->char_count);
    if (this->colors == nullptr) {
        PLOGF << "Failed to allocate memory for colors";
        return;
    }

    this->faces = (GLushort*) realloc(this->faces, sizeof(GLushort) * 6 * this->char_count);
    if (this->faces == nullptr) {
        PLOGF << "Failed to allocate memory for faces";
        return;
    }

    for (unsigned int i = 0; i < this->char_count; ++i) {
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

void TextLayer::set_start_line(unsigned int line_num) {
    this->last_start_line = this->start_line;
    this->start_line = line_num;
}

unsigned int TextLayer::get_start_line() {
    return this->start_line;
}

void TextLayer::calculate_attribute_buffers() {
    this->calculate_dimensions();

    float bearing_x, bearing_y, width, height, advance, x_pos, y_pos;
    float to_screen_width = 2.0f / Window::width;
    float to_screen_height = 2.0f / Window::height;
    float space_advance = glyphs[' '].advance / 64.0f * to_screen_width;
    float font_height = this->font_height * to_screen_height;

    // Set up all the VBOs

    this->buffer->seek_line(this->start_line);

    int line_diff = this->start_line - this->last_start_line;

    if (line_diff > 0) {
        // We've shifted down in the document by `line_diff` lines.
        // Visually, the document is moving upwards by `line_diff` lines.
        // We can replace the top-most `line_diff` lines in memory.

        // Here's an example shift down by 2 lines:
        // Memory before: a b c d e
        // Memory after:  f g c d e

        unsigned int lines_replaced = 0;
        std::optional<string> line;
        char c;

        while ((line = this->buffer->read_next_line()).has_value() && lines_replaced < line_diff) {
            PLOGD << "Got line: \"" << *line << "\"";
            float last_x = -1.0f;

            for (unsigned int column = 0; column < this->columns;) {
                if (column < line->length()) {
                    c = (*line)[column];
                } else {
                    // This is our clear character
                    c = ' ';
                }

                Glyph glyph = glyphs[c];
                unsigned int idx = (this->top_line_idx * this->columns + column) * 4;

                // Handle geometry correctly for whitespace characters
                if (c == '\r') {
                    last_x = -1.0f;
                    column = 0;
                    continue;
                } else if (c == '\t') {
                    last_x += 4.0f * space_advance;
                    column += 4;
                    continue;
                }

                if (column < columns) {
                    advance = glyph.advance / 64.0f * to_screen_width;
                    bearing_x = float(glyph.bearing.x) * to_screen_width;
                    bearing_y = float(glyph.bearing.y) * to_screen_height;
                    width = float(glyph.size.x) * to_screen_width;
                    height = float(glyph.size.y) * to_screen_height;

                    x_pos = last_x + bearing_x;
                    y_pos = last_y + bearing_y - font_height;

                    VEC2(vertices, idx + 0, x_pos,         y_pos - height)
                    VEC2(vertices, idx + 1, x_pos,         y_pos)
                    VEC2(vertices, idx + 2, x_pos + width, y_pos)
                    VEC2(vertices, idx + 3, x_pos + width, y_pos - height)

                    VEC2(uvs, idx + 0, glyph.uv_start.x, glyph.uv_stop.y)
                    VEC2(uvs, idx + 1, glyph.uv_start.x, glyph.uv_start.y)
                    VEC2(uvs, idx + 2, glyph.uv_stop.x,  glyph.uv_start.y)
                    VEC2(uvs, idx + 3, glyph.uv_stop.x,  glyph.uv_stop.y)

                    VEC4(colors, idx + 0, 1.0f, 1.0f, 1.0f, 1.0f)
                    VEC4(colors, idx + 1, 1.0f, 1.0f, 1.0f, 1.0f)
                    VEC4(colors, idx + 2, 1.0f, 1.0f, 1.0f, 1.0f)
                    VEC4(colors, idx + 3, 1.0f, 1.0f, 1.0f, 1.0f)

                    last_x += advance;
                    column++;
                }
            }

            lines_replaced++;
            this->vertical_char_offset++;
            this->last_y -= font_height;
            this->top_line_idx++;
            this->top_line_idx %= this->rows;
        }
    } else if (line_diff < 0) {
        // BUG: This doesn't work. Like, at all.
 
        // We've shifted up in the document by `line_diff` lines.
        // Same situation as above, but reversed.

        // Here's an example shift up by 2 lines:
        // Memory before: c d e f g
        // Memory after:  c d e a b

        unsigned int lines_replaced = 0;
        std::optional<string> line;
        char c;

        while ((line = this->buffer->read_next_line()).has_value() && lines_replaced < line_diff) {
            PLOGD << "Got line: \"" << *line << "\"";
            float last_x = -1.0f;

            for (unsigned int column = 0; column < this->columns;) {
                if (column < line->length()) {
                    c = (*line)[column];
                } else {
                    // This is our clear character
                    c = ' ';
                }

                Glyph glyph = glyphs[c];
                unsigned int idx = (this->bottom_line_idx * this->columns + column) * 4;

                // Handle geometry correctly for whitespace characters
                if (c == '\r') {
                    last_x = -1.0f;
                    column = 0;
                    continue;
                } else if (c == '\t') {
                    last_x += 4.0f * space_advance;
                    column += 4;
                    continue;
                }

                if (column < columns) {
                    advance = glyph.advance / 64.0f * to_screen_width;
                    bearing_x = float(glyph.bearing.x) * to_screen_width;
                    bearing_y = float(glyph.bearing.y) * to_screen_height;
                    width = float(glyph.size.x) * to_screen_width;
                    height = float(glyph.size.y) * to_screen_height;

                    x_pos = last_x + bearing_x;
                    y_pos = last_y + bearing_y - font_height;

                    VEC2(vertices, idx + 0, x_pos,         y_pos - height)
                    VEC2(vertices, idx + 1, x_pos,         y_pos)
                    VEC2(vertices, idx + 2, x_pos + width, y_pos)
                    VEC2(vertices, idx + 3, x_pos + width, y_pos - height)

                    VEC2(uvs, idx + 0, glyph.uv_start.x, glyph.uv_stop.y)
                    VEC2(uvs, idx + 1, glyph.uv_start.x, glyph.uv_start.y)
                    VEC2(uvs, idx + 2, glyph.uv_stop.x,  glyph.uv_start.y)
                    VEC2(uvs, idx + 3, glyph.uv_stop.x,  glyph.uv_stop.y)

                    VEC4(colors, idx + 0, 1.0f, 1.0f, 1.0f, 1.0f)
                    VEC4(colors, idx + 1, 1.0f, 1.0f, 1.0f, 1.0f)
                    VEC4(colors, idx + 2, 1.0f, 1.0f, 1.0f, 1.0f)
                    VEC4(colors, idx + 3, 1.0f, 1.0f, 1.0f, 1.0f)

                    last_x += advance;
                    column++;
                }
            }

            lines_replaced++;
            this->vertical_char_offset++;
            this->last_y -= font_height;
            this->bottom_line_idx = this->bottom_line_idx ? this->bottom_line_idx - 1 : this->rows - 1;
        }
    } else {
        // We don't need to do anything and
        // this case probably shouldn't occur.
    }

    // Populate buffers

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_vertices);
    glBufferData(
        GL_ARRAY_BUFFER,
        4 * 2 * this->char_count * sizeof(float),
        this->vertices,
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_uvs);
    glBufferData(
        GL_ARRAY_BUFFER,
        4 * 2 * this->char_count * sizeof(float),
        this->uvs,
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo_colors);
    glBufferData(
        GL_ARRAY_BUFFER,
        4 * 4 * this->char_count * sizeof(float),
        this->colors,
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ibo_faces);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        6 * this->char_count * sizeof(GLushort),
        this->faces,
        GL_STATIC_DRAW
    );
}

void TextLayer::draw() {
    float to_screen_height = 2.0f / Window::height;
    float font_height = this->font_height * to_screen_height;
    glm::vec3 translation(
        0.0f,
        font_height * (this->vertical_char_offset - this->rows),
        0.0f
    );
    glm::mat4 model = glm::translate(glm::mat4(1.0f), translation);

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
