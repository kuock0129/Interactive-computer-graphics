#version 300 es
layout(location=0) in vec4 position;
layout(location=1) in vec4 color;

out vec4 vColor;
uniform mat4 mv;

void main() {
    gl_Position = mv * position;
    vColor = color;
}