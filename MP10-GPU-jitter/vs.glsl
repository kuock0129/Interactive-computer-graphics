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
    float scale = 0.05;
    float x = (position.x - 8.25) * scale;
    float y = -(position.y - 12.5) * scale;

    // Use more varied frequencies and phase shifts for more dynamic movement
    float vertexPhase = random(float(gl_VertexID)) * 6.28318; // 2*PI for full rotation
    
    // Generate unique jitter for each vertex with multiple frequency components
    float xJitter = random(float(gl_VertexID) + 3.3) * 0.02 * 
                    (cos(seconds * 15.0 + vertexPhase) + 
                     0.5 * sin(seconds * 13.0 + vertexPhase * 2.0));
                     
    float yJitter = random(float(gl_VertexID) + 12.7) * 0.05 * 
                    (cos(seconds * 27.0 + vertexPhase) + 
                     0.7 * sin(seconds * 17.0 + vertexPhase * 2.0));
    
    gl_Position = vec4(x + xJitter, y + yJitter, 0.0, 1.0);
}