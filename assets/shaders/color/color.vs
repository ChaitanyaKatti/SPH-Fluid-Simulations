#version 330 core
precision highp float;

layout(location = 0) in vec3 aPos;

uniform vec4 color;
uniform mat4 modelMatrix;
uniform mat4 viewProjMatrix;

void main()
{
    vec3 FragPos = vec3(modelMatrix * vec4(aPos, 1.0));
    gl_Position = viewProjMatrix * vec4(FragPos, 1.0);
}
