#version 300 es

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;

uniform float seconds;

out vec4 vColor;

void main() {
    vColor = color;
    
    // Scale and center the coordinates
    float scale = 0.05;  // Scale factor to make icon smaller
    
    // Center the icon by subtracting the midpoint of its coordinate range
    float x = (position.x - 8.25) * scale;  // 8.25 is roughly the middle of [0, 16.5]
    float y = -(position.y - 12.5) * scale; // 12.5 is roughly the middle of [0, 25], negative to flip Y
    
    gl_Position = vec4(x, y, 0.0, 1.0);
}