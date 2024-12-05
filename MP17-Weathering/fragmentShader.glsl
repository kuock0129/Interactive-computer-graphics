#version 300 es
precision highp float;

uniform vec4 color;   
uniform vec3 lightdir;  
uniform vec3 lightcolor;
uniform vec3 eye;       

in vec4 pos;
in vec3 vnormal;

out vec4 fragColor;

void main() {
    vec3 n = normalize(vnormal);
    
    float lambert = max(dot(n, lightdir), 0.0);

    vec3 viewDir = normalize(eye - pos.xyz);
    vec3 halfway = normalize(lightdir + viewDir);
    float blinn = pow(max(dot(n, halfway), 0.0), 50.0);
    
    vec3 diffuse = lightcolor * lambert;
    vec3 specular = lightcolor * blinn;

    fragColor = vec4(color.rgb * (diffuse + specular), color.a);
}