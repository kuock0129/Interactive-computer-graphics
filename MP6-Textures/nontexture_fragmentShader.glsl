#version 300 es
precision highp float;

// global variable
uniform vec4 color;
uniform vec3 lightdir;
uniform vec3 lightcolor;
uniform vec3 halfway;

out vec4 fragColor;
in vec3 vnormal;

void main() {
    vec3 n = normalize(vnormal);
    float lambert = max(dot(n, lightdir), 0.0);
    float blinn = pow(max(dot(n, halfway), 0.0), 50.0);
    // alpha is treated as a an amount of specular shine
    fragColor = vec4(
        color.rgb * (lightcolor * lambert) * (1.0-color.a) + (lightcolor * blinn) * 3.0 * color.a
        , 1.0);
}