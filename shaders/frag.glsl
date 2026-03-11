#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler   u_sampler;
layout(set = 1, binding = 1) uniform texture2D u_texture;

void main() {
    outColor = texture(sampler2D(u_texture, u_sampler), fragUV);
}