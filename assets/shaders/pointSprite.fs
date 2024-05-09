#version 330 core
precision highp float;
#define PI 3.1415926535897932384626433832795

in vec3 lookdir;

uniform float uTime;
uniform vec3 eyePos;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

uniform sampler2D texture0;

void main() {
    if(length(gl_PointCoord - 0.5) > 0.5) {
        discard;
    }
    vec3 uvPos = 2.0*vec3(gl_PointCoord-0.5, sqrt(0.25 - dot(gl_PointCoord - 0.5, gl_PointCoord - 0.5)));
    uvPos = (mat3(viewMatrix)) * uvPos;
    vec2 uv = vec2((atan(uvPos.z, uvPos.x)) / (2.0 * PI) + 0.5, 
                   (asin(uvPos.y)) / PI + 0.5);
    uv = vec2(1.0 - uv.x, 1.0 - uv.y);
    uv = mod(uv, 1.0);
    gl_FragColor = texture(texture0, uv);
    // gl_FragColor = vec4(1.0, 0.0, 0.33, 1.0);
}