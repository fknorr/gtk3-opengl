#version 130

in vec2 pos;
in vec3 color;

out vec3 var_color;

void main() {
    var_color = color;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 0, 1);
}
