#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

uniform sampler2D texture0;
uniform mat4 model;
uniform mat4 viewProj;
out vec2 TexCoord;

void main()
{
    TexCoord = aTexCoord;
    gl_Position = viewProj * model * vec4(aPos, 1.0);
}
