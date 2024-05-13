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
    vec3 halfDir = normalize(-ligthDir + normalize(fNormal));
    float shade = 0.5 * max(0.0, dot(-ligthDir, modelNormal));
    shade += 0.2 * pow(max(0.0, dot(halfDir, modelNormal)), 32.0);
    shade += 0.3;
    fragColor = vec4(fColor*shade,  1.0);
    // fragColor = vec4(fColor,  1.0);
    
    // Write depth to depth buffer
    vec4 NCD = projMatrix * viewMatrix * vec4(fPos + pointSize * modelNormal, 1.0);
    float depth = (1 + NCD.z / NCD.w) / 2;
    gl_FragDepth = depth;
}