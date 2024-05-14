#version 330 core
precision highp float;

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;

uniform sampler2D uColorTexture;
uniform sampler2D uDepthTexture;

out vec2 texCoord;

void main()
{
    texCoord = uv;
    gl_Position = vec4(position, 1.0);
}