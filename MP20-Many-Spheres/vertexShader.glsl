#version 300 es
layout(location=0) in vec4 position;
layout(location=1) in vec3 normal;
out vec3 vnormal;

uniform mat4 mv;
uniform mat4 p;


void main() {
    gl_Position = p * mv * position;
    vnormal = normal; // no need to consider rotation
}