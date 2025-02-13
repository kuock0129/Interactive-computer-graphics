#version 300 es
precision highp float;
layout(location=0) in vec4 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 texcoord;

uniform mat4 mv; // global variable
uniform mat4 p;

out vec3 vnormal;
out vec2 vtexcoord;
out vec4 pos;

void main() {
    gl_Position = p * mv * position;
    pos = position;
    vnormal = mat3(mv) * normal;
    vtexcoord = texcoord;
}