#version 300 es
precision highp float;

// Uniform inputs
uniform vec4 color;      // Object base color
uniform vec3 lightdir;   // Light direction vector
uniform vec3 lightcolor; // Light color
uniform vec3 eye;        // Eye/camera position

// Vertex inputs
in vec4 pos;
in vec3 vnormal;

// Fragment output
out vec4 fragColor;

void main() {
    vec3 n = normalize(vnormal);
    
    // Lambert diffuse lighting calculation
    float lambert = max(dot(n, lightdir), 0.0);
    
    // Blinn-Phong specular highlights
    vec3 viewDir = normalize(eye - pos.xyz);
    vec3 halfway = normalize(lightdir + viewDir);
    float blinn = pow(max(dot(n, halfway), 0.0), 50.0);
    
    vec3 diffuse = lightcolor * lambert;
    vec3 specular = lightcolor * blinn;
    
    // Final fragment color
    fragColor = vec4(color.rgb * (diffuse + specular), color.a);
}