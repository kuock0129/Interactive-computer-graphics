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
    
    // Get the slope steepness from the y (up) component of the normal
    float steepness = n.y; // 1.0 for flat, 0.0 for vertical
    
    // Define material colors
    vec3 shallowColor = vec3(0.2, 0.6, 0.1);  // Green for shallow slopes
    vec3 steepColor = vec3(0.6, 0.3, 0.3);    // Red for steep slopes
    
    // Use step function for crisp transition at 0.7 steepness
    vec3 materialColor = mix(steepColor, shallowColor, step(0.7, steepness));
    
    // Lambert diffuse lighting calculation
    float lambert = max(dot(n, lightdir), 0.0);
    
    // Specular parameters
    float shallowShininess = 100.0;  // Sharper spots for green areas
    float steepShininess = 30.0;     // Broader spots for red areas
    float shininess = mix(steepShininess, shallowShininess, step(0.1, steepness));
    
    float shallowSpecIntensity = 10.0;  // Brighter highlights for green
    float steepSpecIntensity = 0.5;    // Dimmer highlights for red
    float specIntensity = mix(steepSpecIntensity, shallowSpecIntensity, step(0.7, steepness));
    
    // Blinn-Phong specular highlights
    vec3 viewDir = normalize(eye - pos.xyz);
    vec3 halfway = normalize(lightdir + viewDir);
    float specAngle = max(dot(n, halfway), 0.0);
    float specular = pow(specAngle, shininess) * specIntensity;
    
    // Keep specular color separate from material color
    vec3 diffuse = materialColor * lightcolor * lambert;
    vec3 specularColor = lightcolor * specular;
    
    // Final color with enhanced highlights
    vec3 finalColor = diffuse + specularColor;
    
    fragColor = vec4(finalColor, 1.0);
}
