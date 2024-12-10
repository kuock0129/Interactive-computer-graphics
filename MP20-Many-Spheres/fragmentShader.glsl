#version 300 es
precision highp float;

uniform vec3 lightdir;
uniform vec3 lightcolor;
uniform vec3 halfway;

out vec4 fragColor;
in vec4 vcolor;

void main() {
    vec2 cood = gl_PointCoord * 2.0 - vec2(1.0,1.0);
    float nx = cood.x, ny = cood.y;
    if (length(cood) > 1.0) discard;
    float nz = sqrt(1.0 - nx*nx - ny*ny);
    vec3 n = vec3(nx, ny, nz);

    float lambert = max(dot(n, lightdir), 0.0);
    float specular = pow(max(dot(n, halfway), 0.0), 50.0);
    
    fragColor = vec4(
        vcolor.rgb * (lightcolor * lambert) + lightcolor * specular * 0.3, 
        vcolor.a
    );
}