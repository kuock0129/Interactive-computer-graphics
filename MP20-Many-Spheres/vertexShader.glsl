#version 300 es
layout(location=0) in vec4 position;
layout(location=1) in float radius;
layout(location=2) in vec4 color;

uniform mat4 mv;
uniform mat4 p;
uniform float x;
uniform float projx;

out vec4 vcolor;

void main() {
    gl_Position = p * mv * position;
    gl_PointSize = x * projx * radius / gl_Position.w;
    vcolor = color;
}