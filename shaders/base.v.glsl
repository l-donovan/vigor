uniform vec3 myThing;

in vec3 vertex;
in vec3 color;

out vec4 v_color;

void main() {
    gl_Position = vec4(vertex, 1.0);
    v_color = vec4(color, 1.0);
}
