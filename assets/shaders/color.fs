#version 330 core
precision highp float;

in vec3 FragPos;
in vec3 Normal;
// in vec2 TexCoord;

uniform vec3 color;

out vec4 FragColor;

void main() {
    float shade = max(0.0, dot(normalize(Normal), normalize(vec3(0.0, 0.0, 1.0))));
    FragColor = vec4(color, 1.0);
}
