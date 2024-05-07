#version 330 core

in vec2 TexCoord;

uniform sampler2D texture0;

out vec4 FragColor;

void main() {
    FragColor = vec4(texture(texture0, TexCoord).rgb, 1.0);
}
