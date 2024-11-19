#version 300 es
layout(location=0) in vec4 position;
uniform mat4 mv;
uniform mat4 p;
void main() {
    gl_Position = p * mv * position;
}