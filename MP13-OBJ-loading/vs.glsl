#version 300 es
layout(location=0) in vec4 position;
layout(location=1) in vec2 texture;
layout(location=2) in vec3 normal;
layout(location=3) in vec3 color;

uniform mat4 mv;
uniform mat4 p;
out vec3 vnormal;
out vec2 texture_coordinate;
out vec3 vcolor;


void main() {
    gl_Position = p * mv * position;
    vnormal = mat3(mv) * normal;
    texture_coordinate = texture;
    vcolor = color;
}