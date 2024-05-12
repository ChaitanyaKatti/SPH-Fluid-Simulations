#version 330 core
precision highp float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

uniform float uTime;
uniform vec3 eyePos;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

out vec3 color;
out vec3 lookdir;

void main()
{
    color = aColor;
    lookdir = normalize(aPos - eyePos);

    vec4 pos = viewMatrix * modelMatrix * vec4(aPos, 1.0);
    gl_Position = projMatrix * pos;
    gl_PointSize = -50.0 / pos.z;
}
