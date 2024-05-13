#version 330 core
precision highp float;
#define PI 3.1415926535897932384626433832795

in vec3 color;
in vec3 lookdir;

uniform float uTime;
uniform vec3 eyePos;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

const vec3 ligthDir = normalize(vec3(1.0, 1.0, 0.0));

void main() {
    if(length(gl_PointCoord - 0.5) > 0.5) {
        discard;
    }
    vec2 uv = gl_PointCoord.xy;
    uv.y = 1.0 - uv.y;
    uv = uv * 2.0 - 1.0;
    vec3 modelNormal = vec3(uv, sqrt(1.0 - dot(uv, uv)));
    modelNormal = normalize(inverse(mat3(viewMatrix * modelMatrix)) * modelNormal);
    vec3 halfDir = normalize(-lookdir + ligthDir);
    
    float shade = max(0.0, dot(ligthDir, modelNormal));
    shade += 0.2*pow(max(0.0, dot(halfDir, modelNormal)), 32.0);
    shade += 0.1;
    
    gl_FragColor = vec4(color*shade,  1.0);
}