#version 330 core
precision highp float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

uniform float uTime;
uniform float pointSize;
uniform vec3 eyePos;
// uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

out VS_OUT {
    vec3 color;
    vec3 normal;
} vs_out;

void main()
{
    vs_out.color = aColor;
    vs_out.normal = normalize(eyePos - aPos);
    gl_Position = vec4(aPos, 1.0);
}
