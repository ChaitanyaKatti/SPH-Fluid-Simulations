#version 330 core
precision highp float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform vec3 color;
uniform mat4 modelMatrix;
uniform mat4 viewProjMatrix;
uniform mat3 normalMatrix;

out vec3 FragPos;
out vec3 Normal;
// out vec2 TexCoord;

void main()
{
    FragPos = vec3(modelMatrix * vec4(aPos, 1.0));
    Normal = normalMatrix * aNormal;
    // TexCoord = aTexCoord;
    gl_Position = viewProjMatrix * vec4(FragPos, 1.0);
}
