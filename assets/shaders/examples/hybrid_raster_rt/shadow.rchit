#version 460 core
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT uint hitCounter;

void main()
{
    hitCounter = 3;
}
