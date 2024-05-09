#version 330 core
precision highp float;

layout(location = 0) in vec3 aPos;

uniform float uTime;
uniform vec3 eyePos;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

out vec3 lookdir;

void main()
{
    vec4 pos = viewMatrix * modelMatrix * vec4(aPos, 1.0);
    gl_Position = projMatrix * pos;
    gl_PointSize = -1000.0 / pos.z;

    // Calculate the yaw and pitch made by line joining the eye position and the vertex
    lookdir = normalize(aPos - eyePos);
}
