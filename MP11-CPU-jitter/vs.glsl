#version 300 es

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;

uniform float seconds;

out vec4 vColor;

void main() {
    vColor = color;
    
    // Scale and center the coordinates
    float scale = 0.05;
    float x = (position.x - 8.25) * scale;
    float y = -(position.y - 12.5) * scale;
    
    gl_Position = vec4(x, y, 0.0, 1.0);
}