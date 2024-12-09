#version 300 es
precision highp float;


uniform vec4 color;
uniform vec3 lightdir;
uniform vec3 lightcolor;
uniform vec3 halfway;
uniform vec3 lightdir2;
uniform vec3 lightcolor2;
uniform vec3 halfway2;


out vec4 fragColor;
in vec3 vnormal;


void main() {
    vec3 n = normalize(vnormal);
    float lambert = max(dot(n, lightdir), 0.0);
    float blinn = pow(max(dot(n, halfway), 0.0), 50.0);

    // float lambert2 = max(dot(n, lightdir2), 0.0);
    // float blinn2 = pow(max(dot(n, halfway2), 0.0), 50.0);


    fragColor = vec4(
        color.rgb * 
        (lightcolor * lambert  * 0.5) + (lightcolor * blinn * 0.5)
        , color.a);
}