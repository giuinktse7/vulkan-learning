#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in float fragOpacity;

layout(location = 0) out vec4 outColor;

void main() {
    // vec4 texel = texture(texSampler, fragTexCoord);
    // outColor.rgb = texel.rgb * fragColor.rgb * fragColor.a * texel.a;
    // outColor.a = texel.a * fragColor.a * fragOpacity;
    outColor = texture(texSampler, fragTexCoord);
    // outColor.rgb = texel.rgb;
    // outColor.a = texel.a;
    // outColor.rgb = vec3(1.0, 0.0, 1.0) * texel.a;
    // outColor.a = texel.a;
}
