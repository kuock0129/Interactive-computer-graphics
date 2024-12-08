#version 300 es
precision highp float;

uniform vec3 lightdir;
uniform vec3 lightcolor;
uniform vec3 halfway;
uniform sampler2D image;

out vec4 fragColor;
in vec3 vnormal;
in vec2 texture_coordinate;
void main() {
    vec4 color = texture(image, texture_coordinate);
    vec3 n = normalize(vnormal);
    float lambert = max(dot(n, lightdir), 0.0);
    // alpha is treated as a an amount of specular shine
    fragColor = vec4(
        color.rgb * (lightcolor * lambert)
        , color.a);
}