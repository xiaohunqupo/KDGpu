#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 color;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) in vec3 worldPosition;
layout(location = 3) in vec3 worldView;

layout(early_fragment_tests) in;

struct AlphaFragment {
    vec4 color;
    float depth;
    uint next;
    float _pad[2];
};

// Alpha
layout(std430, set = 1, binding = 0) coherent buffer AlphaFragments
{
    uint nextIdx;
    vec3 _pad;
    AlphaFragment fragments[];
}
alphaFragments;

layout(set = 1, binding = 1, r32ui) uniform coherent uimage2D alphaHeadPointer;

layout(push_constant) uniform PushConstants
{
    vec3 lightWorldPos;
}
pushConstants;

vec4 adsModel(vec4 color)
{
    const float shininess = 50.0;

    // We perform all work in world space
    vec3 n = normalize(worldNormal);
    vec3 lightDir = -pushConstants.lightWorldPos;
    vec3 s = normalize(-lightDir);
    float sDotN = dot(s, n);

    // Calculate the diffuse factor
    float diffuse = max(sDotN, 0.0);

    // Calculate the specular factor
    float specular = 0.0;
    if (diffuse > 0.0 && shininess > 0.0) {
        float normFactor = (shininess + 2.0) / 2.0;
        vec3 r = reflect(s, n); // Reflection direction in world space
        specular = normFactor * pow(max(dot(r, worldView), 0.0), shininess);
    }

    // Accumulate the diffuse and specular contributions
    const vec3 lightDiffuseColor = vec3(1.0);
    const vec3 lightSpecularColor = vec3(0.2);
    vec3 diffuseFactor = diffuse * lightDiffuseColor;
    vec3 specularFactor = specular * lightSpecularColor;

    vec3 ambientColor = color.rgb * 0.4;
    const vec3 specularColor = vec3(1.0);
    vec4 lightedColor = vec4(ambientColor + diffuseFactor * color.rgb + specularFactor * specularColor, color.a);
    return lightedColor;
}

void main()
{
    // Get next free entry in fragments buffers
    // We treat 0 as the end of the linked list so we offset every value by 1
    uint nodeIdx = atomicAdd(alphaFragments.nextIdx, 1) + 1;

    // If we still have room in the fragments buffers
    if (nodeIdx < alphaFragments.fragments.length()) {

        // Insert new fragment entry
        alphaFragments.fragments[nodeIdx].color = adsModel(color);
        alphaFragments.fragments[nodeIdx].depth = gl_FragCoord.z;

        // Update alphaHeadPointer to nodeIdx for current fragment
        uint previousHeadIdx =
                imageAtomicExchange(alphaHeadPointer, ivec2(gl_FragCoord.xy), nodeIdx);
        // Set next to previousHeadIdx (0 is considered as the ending index)
        alphaFragments.fragments[nodeIdx].next = previousHeadIdx;
    }
}
