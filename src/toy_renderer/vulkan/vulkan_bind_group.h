#pragma once

#include <toy_renderer/api/api_bind_group.h>
#include <toy_renderer/handle.h>
#include <vulkan/vulkan.h>

namespace ToyRenderer {

class VulkanResourceManager;
struct Device_t;

struct VulkanBindGroup : public ApiBindGroup {
    explicit VulkanBindGroup(VkDescriptorSet _descriptorSet,
                             VulkanResourceManager *_vulkanResourceManager,
                             const Handle<Device_t> &_deviceHandle);

    VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
    VulkanResourceManager *vulkanResourceManager;
    Handle<Device_t> deviceHandle;
};

} // namespace ToyRenderer
