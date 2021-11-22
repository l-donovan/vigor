#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "vigor/text_layer.h"
#include "vigor/text_renderer.h"
#include "vigor/window.h"

#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <string>

// TODO: Remove after testing
#include <chrono>
using namespace std::chrono;

using std::string;

std::map<GLchar, Character> characters;

void TextLayer::setup() {
    this->renderer.set_font(
            "/Users/ldonovan/Library/Fonts/Blex Mono Medium Nerd Font Complete Mono.ttf",
            this->char_height);
    this->renderer.load();

    return;

    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft)) {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return;
    }

    // Find path to font
    string font_name = "/Users/ldonovan/Library/Fonts/Blex Mono Medium Nerd Font Complete Mono.ttf";
    if (font_name.empty()) {
        std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;
        return;
    }

    // Load font as face
    FT_Face face;
    if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return;
    } else {
        // set size to load glyphs as
        FT_Set_Pixel_Sizes(face, 0, this->char_height);

        // disable byte-alignment restriction
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // load first 128 characters of ASCII set
        for (unsigned char c = 0; c < 128; c++) {
            // Load character glyph
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }
            // generate texture
            unsigned int texture;
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
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // Configure VAO/VBO for texture quads
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    this->update();
}

TextLayer::~TextLayer() {
    glDeleteVertexArrays(1, &this->VAO);
    glDeleteBuffers(1, &this->VBO);
}

void TextLayer::set_text(string text) {
    this->text = text;
    this->update();
}

void TextLayer::set_position(float x, float y) {
    this->x = x * Window::width;
    this->y = (1.0f - y) * Window::height - this->scale * this->char_height;
    this->recalculate_visibility();
}

void TextLayer::recalculate_visibility() {
}

void TextLayer::recalculate_visibility_2() {
    auto start = high_resolution_clock::now();

    float x = this->x;
    float y = this->y;

    // Iterate through all characters
    bool invisible_until_eol = false;
    bool invisible_until_eof = false;
    for (int i = 0; i < this->text.size(); ++i) {
        char c = this->text[i];
        Character ch = characters[c];
        bool visible = true;

        float x_pos = x + ch.bearing.x * this->scale;
        float y_pos = y - (ch.size.y - ch.bearing.y) * this->scale;

        float w = ch.size.x * this->scale;
        float h = ch.size.y * this->scale;

        if (x_pos > Window::width) {
            invisible_until_eol = true;
            visible = false;
        }

        if (y_pos > Window::height) {
            visible = false;
        }

        if ((y_pos + ch.bearing.y) < 0) {
            invisible_until_eof = true;
        }

        if (c == '\n' and invisible_until_eof) {
            for (int j = i; j < this->geometry.size(); ++j)
                this->geometry[j].visible = false;
            break;
        }

        if (c == '\n') {
            invisible_until_eol = false;
            x = this->x;
            y -= this->char_height * this->scale;
        } else if (c == '\r') {
            x = this->x;
        } else {
            // Advance cursors for next glyph (note that advance is number of 1/64 pixels)
            x += (ch.advance >> 6) * this->scale; // bitshift by 6 to get value in pixels (2^6 = 64)
        }

        // Update VBO for each character
        float verts[6][4] = {
            { x_pos,     y_pos + h,   0.0f, 0.0f },
            { x_pos,     y_pos,       0.0f, 1.0f },
            { x_pos + w, y_pos,       1.0f, 1.0f },

            { x_pos,     y_pos + h,   0.0f, 0.0f },
            { x_pos + w, y_pos,       1.0f, 1.0f },
            { x_pos + w, y_pos + h,   1.0f, 0.0f }
        };

        memcpy(&this->geometry[i].vertices, verts, 6 * 4 * sizeof(float));
        this->geometry[i].visible = invisible_until_eol ? false : visible;
    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    std::cout << "recalculate_visibility took " << duration.count() << "µs" << std::endl;
}

void TextLayer::update() {
    float x = this->x;
    float y = this->y;

    glm::mat4 projection = glm::ortho(
        0.0f,
        static_cast<float>(Window::width),
        0.0f,
        static_cast<float>(Window::height)
    );
    glUniformMatrix4fv(glGetUniformLocation(this->shader_id, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    this->geometry.clear();

    // Iterate through all characters
    std::string::const_iterator c;
    for (c = this->text.begin(); c != this->text.end(); ++c) {
        Character ch = characters[*c];
        bool visible = true;

        float x_pos = x + ch.bearing.x * this->scale;
        float y_pos = y - (ch.size.y - ch.bearing.y) * this->scale;

        float w = ch.size.x * this->scale;
        float h = ch.size.y * this->scale;

        if (y_pos > Window::height || x_pos > Window::width) {
            visible = false;
        }

        if ((y_pos + ch.bearing.y) < 0) {
            visible = false;
        }

        if (*c == '\n') {
            x = this->x;
            y -= this->char_height * this->scale;
        } else if (*c == '\r') {
            x = this->x;
        } else {
            // Update VBO for each character
            CharacterGeometry cg = {
                {
                    { x_pos,     y_pos + h,   0.0f, 0.0f },
                    { x_pos,     y_pos,       0.0f, 1.0f },
                    { x_pos + w, y_pos,       1.0f, 1.0f },

                    { x_pos,     y_pos + h,   0.0f, 0.0f },
                    { x_pos + w, y_pos,       1.0f, 1.0f },
                    { x_pos + w, y_pos + h,   1.0f, 0.0f }
                },
                ch.texture_id,
                visible
            };

            this->geometry.push_back(cg);

            // Advance cursors for next glyph (note that advance is number of 1/64 pixels)
            x += (ch.advance >> 6) * this->scale; // bitshift by 6 to get value in pixels (2^6 = 64)
        }
    }
}

void TextLayer::draw() {
    glm::vec3 color(0.5, 0.8f, 0.2f);

    glUniform3fv(glGetUniformLocation(this->shader_id, "textColor"), 1, glm::value_ptr(color));

    glActiveTexture(GL_TEXTURE0);

    float time_val = glfwGetTime();
    float green_val = sin(time_val) / 2.0f + 0.5f;
    glUniform1f(glGetUniformLocation(this->shader_id, "animationProgress"), green_val);

    glm::mat4 projection = glm::ortho(
        0.0f,
        static_cast<float>(Window::width),
        0.0f,
        static_cast<float>(Window::height)
    );
    glUniformMatrix4fv(glGetUniformLocation(this->shader_id, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(this->VAO);

    for (CharacterGeometry cg : this->geometry) {
        if (!cg.visible)
            continue;

        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, cg.texture_id);

        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(cg.vertices), cg.vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
