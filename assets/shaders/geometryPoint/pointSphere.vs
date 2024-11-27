#version 330 core
precision highp float;

struct Camera {
    vec3 eyePos;
    mat4 viewMatrix;
    mat4 projMatrix;
};

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

uniform float pointSize;
uniform Camera camera;

out VS_OUT {
    vec3 color;
    vec3 normal;
} vs_out;

void main()
{
    vs_out.color = aColor;
    vs_out.normal = normalize(camera.eyePos - aPos);
    gl_Position = vec4(aPos, 1.0);
}
