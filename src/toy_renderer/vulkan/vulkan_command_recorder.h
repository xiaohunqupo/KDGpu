#pragma once

#include <toy_renderer/api/api_command_recorder.h>

#include <toy_renderer/handle.h>

#include <vulkan/vulkan.h>

namespace ToyRenderer {

class VulkanResourceManager;

struct Device_t;
struct Buffer_t;

struct VulkanCommandRecorder : public ApiCommandRecorder {
    explicit VulkanCommandRecorder(VkCommandPool _commandPool,
                                   const Handle<CommandBuffer_t> _commandBufferHandle,
                                   VulkanResourceManager *_vulkanResourceManager,
                                   const Handle<Device_t> &_deviceHandle);

    void begin() final;
    void copyBuffer(const BufferCopy &copy) final;
    void memoryBarrier(const MemoryBarrierOptions &options) final;
    void bufferMemoryBarrier(const BufferMemoryBarrierOptions &options) final;
    void executeSecondaryCommandBuffer(const Handle<CommandBuffer_t> &secondaryCommandBuffer) final;
    Handle<CommandBuffer_t> finish() final;

    VkCommandPool commandPool{ VK_NULL_HANDLE };
    VkCommandBuffer commandBuffer{ VK_NULL_HANDLE };
    Handle<CommandBuffer_t> commandBufferHandle;
    VulkanResourceManager *vulkanResourceManager{ nullptr };
    Handle<Device_t> deviceHandle;
};

} // namespace ToyRenderer
