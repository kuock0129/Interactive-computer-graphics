#version 300 es
precision highp float;
out vec4 position;

void main() {
    // Create a single fullscreen quad using normalized device coordinates
    float x = (gl_VertexID == 1 || gl_VertexID == 4 || gl_VertexID == 5) ? 1.0 : -1.0;
    float y = (gl_VertexID == 2 || gl_VertexID == 3 || gl_VertexID == 5) ? 1.0 : -1.0;
    gl_Position = vec4(x, y, 0.0, 1.0);
    position = gl_Position;
}