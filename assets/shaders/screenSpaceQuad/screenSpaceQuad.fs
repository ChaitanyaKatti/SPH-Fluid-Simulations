#version 330 core
precision highp float;

in vec2 texCoord;

uniform sampler2D uColorTexture;
uniform sampler2D uDepthTexture;

out vec4 FragColor;

const vec3 lightDir = -normalize(vec3(1.0, 1.0, 1.0));


void main() {
    vec4 col = texture(uColorTexture, texCoord);
    vec3 normal = col.xyz * 2.0 - 1.0;
    float shade = 0.5 * max(0.0, dot(-lightDir, normal));
    FragColor = vec4(vec3(shade), col.a);



    float depth = texture(uDepthTexture, texCoord).r;
    // FragColor = vec4(depth, depth, depth, 1.0);
    gl_FragDepth = depth;
}
