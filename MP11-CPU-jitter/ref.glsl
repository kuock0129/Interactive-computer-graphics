#version 300 es

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;

uniform float seconds;

out vec4 vColor;


// Hash function to generate pseudo-random values based on vertex ID
float random(float id) {
    return fract(sin(id) * 43758.5453123);
}



void main() {
    vColor = color;
    
    // Scale and center the coordinates
    float scale = 0.05;  // Scale factor to make icon smaller
    
    // Center the icon by subtracting the midpoint of its coordinate range
    float x = (position.x - 8.25) * scale;  // 8.25 is roughly the middle of [0, 16.5]
    float y = -(position.y - 12.5) * scale; // 12.5 is roughly the middle of [0, 25], negative to flip Y

    // // Generate unique jitter for each vertex
    // float xJitter = random(float(gl_VertexID) + 1.3) * 0.02 * sin(seconds*5.0 + float(gl_VertexID) * 0.1);
    // float yJitter = random(float(gl_VertexID) + 2.7) * 0.05 * cos(seconds*17.0 + float(gl_VertexID) * 0.1);
    
    // gl_Position = vec4(x + xJitter, y + yJitter, 0.0, 1.0);
    gl_Position = vec4(x , y, 0.0, 1.0);
}