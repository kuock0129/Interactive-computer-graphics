#version 300 es

layout(location=0) in vec4 position;
layout(location=1) in vec4 color;

uniform float seconds;

out vec4 vColor;

void main() {
    vColor = color;

    float ratio = 0.02 + 0.01 * sin(seconds * 0.6);
    mat4 scale = mat4(
        ratio, 0, 0, 0,
        0, ratio, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );

    float c = cos(seconds*0.8);
    float s = sin(seconds*0.8);
    mat4 rot = mat4(
        c, -s, 0, 0,
        s, c, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );
    float dx = 0.1 * sin(seconds * 1.1), dy = 0.1 * cos(seconds * 2.2);
    vec4 displacement = vec4(dx, dy, 0, 0);

    gl_Position = (rot * scale * position) + displacement;
}