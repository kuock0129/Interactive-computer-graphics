#version 300 es
layout(location=0) in vec4 position;
layout(location=1) in vec3 normal;

uniform mat4 mv;
uniform mat4 p;
out vec3 vnormal;
out vec4 pos;

void main() {
    gl_Position = p * mv * position;
    pos = position;
    vnormal = mat3(mv) * normal;
}