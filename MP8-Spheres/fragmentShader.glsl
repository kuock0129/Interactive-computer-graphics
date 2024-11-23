#version 300 es
precision highp float;

uniform vec4 color;
uniform vec3 lightdir;
uniform vec3 lightcolor;
uniform vec3 halfway;

in vec3 vnormal;

out vec4 fragColor;

void main() {
    vec3 n = normalize(vnormal);
    float lambert = max(dot(n, lightdir), 0.0);
    float blinn = pow(max(dot(n, halfway), 0.0), 50.0);
    fragColor = vec4(
        color.rgb * lightcolor * (lambert+ blinn)
        , color.a);
}