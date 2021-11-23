uniform sampler2D texture;

in vec2 v_uv;
in vec4 v_color;

out vec4 frag_color;

void main() {
    float a = texture2D(texture, v_uv.xy).r;
    frag_color = vec4(v_color.rgb, v_color.a * a);
}