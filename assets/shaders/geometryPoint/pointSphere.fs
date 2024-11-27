#version 330 core
precision highp float;

struct Camera {
    vec3 eyePos;
    mat4 viewMatrix;
    mat4 projMatrix;
};

in vec3 fPos;
in vec2 fUV;
in vec3 fColor;
in vec3 fNormal;
in mat3 axisMatrix;

uniform float pointSize;
uniform Camera camera;

out vec4 fragColor;
// out float gl_FragDepth;

void main() {
    if(length(fUV - 0.5) > 0.5) {
        discard;
    }
    vec3 ligthDir = normalize(vec3(-1.0, -1.0, -1.0));

    vec2 uv = 2*fUV - 1;
    vec3 axisComponents = vec3(uv, sqrt(1.0 - dot(uv, uv)));
    vec3 modelNormal = axisMatrix*axisComponents;
    vec3 halfDir = normalize(-ligthDir + normalize(fNormal));
    float shade = 0.5 * max(0.0, dot(-ligthDir, modelNormal));
    shade += 0.1 * pow(max(0.0, dot(halfDir, modelNormal)), 32.0);
    shade += 0.3;
    fragColor = vec4(fColor*shade,  1.0);
    
    // Write depth to depth buffer by calulating Normalized Device Coordinates
    vec4 NDC = camera.projMatrix * camera.viewMatrix * vec4(fPos + pointSize * modelNormal, 1.0);
    float depth = (1 + NDC.z / NDC.w) / 2; // NDC.z is in range [-NDC.w, NDC.w], so we normalize it to [0, 1]
    gl_FragDepth = depth;
    
    // fragColor = vec4(vec2(modelNormal), 1.0, 1.0); // Visualize world space normal
    // fragColor = vec4(vec3(depth), 1.0, 1.0); // Visualize world space normal
}