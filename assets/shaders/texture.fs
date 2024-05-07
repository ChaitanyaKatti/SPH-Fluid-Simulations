#version 330 core
precision mediump float;

// in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D texture0;

out vec4 FragColor;

void main() {
    vec3 col = texture(texture0, TexCoord).rgb;
    float shade = max(0.0, dot(normalize(Normal), normalize(vec3(0.0, 0.0, 1.0))));
    col *= shade + 0.1;
    FragColor = vec4(col, 1.0);
}
