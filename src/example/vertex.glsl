#version 130

uniform mat4 projection;
uniform mat4 modelview;
in vec2 pos;
in vec3 color;

out vec3 var_color;

void main() {
    var_color = color;
    gl_Position = projection * modelview * vec4(pos, 0, 1);
}
