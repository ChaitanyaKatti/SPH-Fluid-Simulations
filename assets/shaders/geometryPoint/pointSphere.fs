#version 330 core
precision highp float;

in vec3 fPos;
in vec2 fUV;
in vec3 fColor;
in vec3 fNormal;
in mat3 axisMatrix;

uniform float uTime;
uniform float pointSize;
uniform vec3 eyePos;
// uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

out vec4 fragColor;
// out float gl_FragDepth;

void main() {
    if(length(fUV - 0.5) > 0.5) {
        discard;
    }
    vec3 ligthDir = -normalize(vec3(sin(uTime), 1.0, cos(uTime)));

    vec2 uv = 2*fUV - 1;
    vec3 axisComponents = vec3(uv, sqrt(1.0 - dot(uv, uv)));
    vec3 modelNormal = axisMatrix*axisComponents;
    // vec3 halfDir = normalize(-ligthDir + normalize(fNormal));
    // float shade = 0.5 * max(0.0, dot(-ligthDir, modelNormal));
    // shade += 0.2 * pow(max(0.0, dot(halfDir, modelNormal)), 32.0);
    // shade += 0.3;
    
    // Write depth to depth buffer by calulating Normalized Device Coordinates
    vec4 NDC = projMatrix * viewMatrix * vec4(fPos + pointSize * modelNormal, 1.0);
    float depth = (1 + NDC.z / NDC.w) / 2; // NDC.z is in range [-NDC.w, NDC.w], so we normalize it to [0, 1]
    gl_FragDepth = depth;

    // fragColor = vec4(fColor*shade,  1.0); // Shade with light
    fragColor = vec4(0.5*modelNormal + 0.5, 1.0); // Shade with world space normal
    // fragColor = vec4(vec3(depth), 1.0); // Shade with depth
}