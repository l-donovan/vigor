#pragma once

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <string>
#include <vector>

class TextRenderer {
    private:
        GLuint shader_id;
        GLuint atlas_texture_id;
        GLuint vbo_mesh_vertices, vbo_mesh_uvs, vbo_mesh_colors, ibo_mesh_faces;
        GLuint vao;
        GLint attribute_vertex, attribute_uv, attribute_color;
        std::vector<glm::vec4> vertices;
        std::vector<glm::vec2> uvs;
        std::vector<glm::vec4> colors;
        std::vector<GLushort> faces;
    public:
        TextRenderer();
        ~TextRenderer();

        std::string font_path;
        int font_height;

        void set_font(std::string font_path, int font_height);
        void set_shader(GLuint shader_id);
        bool load();
        void put_char(char ch, int x, int y);
        void draw();
};
