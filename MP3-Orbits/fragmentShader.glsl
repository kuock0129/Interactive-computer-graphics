#version 300 es
precision highp float;
out vec4 fragColor;
in vec4 Color2;
void main() {
    fragColor = Color2;
}