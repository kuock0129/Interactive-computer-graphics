#version 300 es
precision highp float;

uniform vec3 lightdir;
uniform vec3 lightcolor;
uniform vec3 halfway;

out vec4 fragColor;
in vec3 vnormal;
in vec3 vcolor;
void main() {
    vec3 n = normalize(vnormal);
    float lambert = max(dot(n, lightdir), 0.0);
    float blinn = pow(max(dot(n, halfway), 0.0), 50.0);
    fragColor = vec4(
        vcolor.rgb * (lightcolor * lambert) + (lightcolor * blinn) * 0.5
        , 1.0);
}