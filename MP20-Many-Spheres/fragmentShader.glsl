#version 300 es
precision highp float;

uniform vec3 lightdir;
uniform vec3 lightcolor;
uniform vec3 halfway;

out vec4 fragColor;
in vec4 vcolor;
void main() {
    vec2 cood = gl_PointCoord * 2.0 - vec2(1.0,1.0); // map to [-1, 1]
    float nx = cood.x, ny = cood.y;
    if (length(cood) > 1.0) discard;
    float nz = sqrt(1.0 - nx*nx - ny*ny); // camera is [0,0,-1]
    vec3 n = vec3(nx, ny, nz);
    float lambert = max(dot(n, lightdir), 0.0);
    float blinn = pow(max(dot(n, halfway), 0.0), 50.0);
    fragColor = vec4(
        vcolor.rgb * (lightcolor * lambert) + (lightcolor * blinn) * 0.4
        , vcolor.a);
}