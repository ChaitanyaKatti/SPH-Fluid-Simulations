#version 330 core
precision mediump float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aOffset;

uniform sampler2D texture0;
uniform mat4 modelMatrix;
uniform mat4 viewProjMatrix;
uniform mat3 normalMatrix;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main()
{
    FragPos = vec3(modelMatrix * vec4(aPos, 1.0)) + aOffset;
    Normal = normalMatrix * aNormal;
    TexCoord = aTexCoord;
    gl_Position = viewProjMatrix * vec4(FragPos, 1.0);
}