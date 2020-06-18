#version 450
#extension GL_ARB_separate_shader_objects : enable

const int BM_NONE = 0;
const int BM_BLEND = 1;
const int BM_ADD = 2;
const int BM_ADDX2 = 3;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in ivec4 inData;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out float fragOpacity;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    int blendMode = inData[0];
    float opacity = 0.0f;
    vec4 color = inColor;

    if(blendMode == BM_NONE) {
        opacity = 1.0f;
        color.a = 1.0f;
    } else if(blendMode == BM_BLEND) {
        opacity = color.a;
    } else if(blendMode == BM_ADD) {
        opacity = 0.0f;
    } else if(blendMode == BM_ADDX2) {
        opacity = 0.0f;
        color *= 2.0f;
    }

    vec4 thing = ubo.projection * vec4(inPosition.x, inPosition.y, 0.0, 1.0);

    // OpenGL uses inverted y axis, Vulkan does not
     gl_Position = vec4(thing.x, -thing.y, thing.z, 1.0f);


    fragColor = color;
    fragTexCoord = inTexCoord;
    fragOpacity = opacity;
}


