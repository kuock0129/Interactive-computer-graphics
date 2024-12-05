#version 300 es
layout(location=0) in vec4 position;
layout(location=1) in vec3 normal;

uniform mat4 mv;
uniform mat4 p;

out vec3 vnormal;
out vec4 pos;

void main() {
    gl_Position = p * mv * position;
    pos = mv * position; // Transform to eye space
    vnormal = mat3(transpose(inverse(mv))) * normal; // Correctly transform normals
}
