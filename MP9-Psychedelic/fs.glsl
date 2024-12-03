#version 300 es
precision highp float;

uniform float seconds;
in vec4 position;
out vec4 fragColor;

float polyWave(vec2 p, float t, vec4 coeffs) {
    float x = p.x * 0.3;
    float y = p.y * 1.9;
    
    float x2 = x * x;
    float y2 = y * y;
    float xy = x * y;
    
    // P(x,y) = ax^4 + by^4 + cx^2y^2 + dxy
    float poly = coeffs.x * x2 * x2 + 
                 coeffs.y * y2 * y2 + 
                 coeffs.z * x2 * y2 + 
                 coeffs.w * xy;
                 
    return sin(poly * 87.0 + t * 4.0);
}

void main() {
    vec2 pos = position.xy;
    
    vec4 redCoeffs = vec4(1.0, 0.4, 0.5, 0.3);
    vec4 greenCoeffs = vec4(0.7, 1.0, -0.4, 0.2);
    vec4 blueCoeffs = vec4(1.7, 0.6, 0.8, -0.5);
    
    float r = polyWave(pos, seconds, redCoeffs);
    float g = polyWave(pos, seconds * 1.1, greenCoeffs);
    float b = polyWave(pos, seconds * 0.9, blueCoeffs);
    
    vec3 color = vec3(r, g, b) * 0.5 + 0.5;
    
    fragColor = vec4(color, 1.0);
}