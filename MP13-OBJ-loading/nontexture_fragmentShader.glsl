#version 300 es
precision highp float;

// global variable
uniform vec4 color;
uniform vec3 lightdir;
uniform vec3 lightcolor;
uniform vec3 eye;        // Eye/camera position

// Vertex inputs
in vec4 pos;
in vec3 vnormal;
in vec2 vtexcoord;

out vec4 fragColor;

void main() {
    vec3 n = normalize(vnormal);
    float lambert = max(dot(n, lightdir), 0.0);
    // Blinn-Phong specular highlights
    vec3 viewDir = normalize(eye - pos.xyz);
    vec3 halfway = normalize(lightdir + viewDir);
    float blinn = pow(max(dot(n, halfway), 0.0), 50.0);

    vec3 diffuse = lightcolor * lambert;
    vec3 specular = lightcolor * blinn;

    // alpha is treated as a an amount of specular shine
    fragColor = vec4(
        color.rgb * diffuse * (1.0-color.a) + specular * 3.0 * color.a, 1.0);
}