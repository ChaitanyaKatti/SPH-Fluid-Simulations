#version 330 core
precision highp float;

struct Camera {
    vec3 eyePos;
    mat4 viewMatrix;
    mat4 projMatrix;
};

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in VS_OUT {
    vec3 color;
    vec3 normal;
} gs_in[];  

uniform float pointSize;
uniform Camera camera;

out vec3 fPos;
out vec2 fUV;
out vec3 fColor;
out vec3 fNormal;
out mat3 axisMatrix;

void main() {
    // Pass through
    fPos = gl_in[0].gl_Position.xyz;
    fColor = gs_in[0].color;
    fNormal = gs_in[0].normal;
    
    // Calculate face normal, the normal to the quad
    vec3 faceNormal = normalize(camera.eyePos - gl_in[0].gl_Position.xyz);

    // Calculate axis matrix
    vec3 pos = (gl_in[0].gl_Position).xyz;
    vec3 up = vec3(0, 1, 0);
    vec3 right = normalize(cross(up, faceNormal));
    up = normalize(cross(faceNormal, right));

    // axisMatrix is column vector of right, up, viewNormal
    axisMatrix = mat3(right, up, faceNormal);

    // Calculate quad vertices
    vec3 p0 = pos + (right + up) * pointSize;
    vec3 p1 = pos - (right - up) * pointSize;
    vec3 p2 = pos + (right - up) * pointSize;
    vec3 p3 = pos - (right + up) * pointSize;
    
    // Emit quad
    // 0 1 - Top left
    gl_Position = camera.projMatrix * camera.viewMatrix * vec4(p0, 1.0);
    fUV = vec2(1, 1);
    EmitVertex();
    // 1 3 - Bottom left
    gl_Position = camera.projMatrix * camera.viewMatrix * vec4(p1, 1.0);
    fUV = vec2(0, 1);
    EmitVertex();
    // 2 0 - Top right
    gl_Position = camera.projMatrix * camera.viewMatrix * vec4(p2, 1.0);
    fUV = vec2(1, 0);
    EmitVertex();
    // 3 2 - Bottom right
    gl_Position = camera.projMatrix * camera.viewMatrix * vec4(p3, 1.0);
    fUV = vec2(0, 0);
    EmitVertex();
    
    EndPrimitive();
}    