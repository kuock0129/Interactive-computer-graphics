#version 300 es
precision highp float;

uniform float seconds;

in vec4 verColor;
out vec4 fragColor;

void main() {
    fragColor = verColor;
}