/*
  This file is part of KDGpu.

  SPDX-FileCopyrightText: 2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>

  SPDX-License-Identifier: MIT

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#pragma once

#include <span>
#include <KDGpu/vulkan/vulkan_framebuffer.h>
#include <KDGpu/vulkan/vulkan_render_pass.h>

#include <KDGpu/handle.h>
#include <KDGpu/kdgpu_export.h>
#include <KDGpu/config.h>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <unordered_map>
#include <KDGpu/adapter_features.h>
#include <KDGpu/adapter_queue_type.h>
#include <KDGpu/device_options.h>
#include <KDGpu/queue_description.h>

#if defined(KDGPU_PLATFORM_WIN32)
struct VkSemaphoreGetWin32HandleInfoKHR;
struct VkFenceGetWin32HandleInfoKHR;
#endif

namespace KDGpu {

class VulkanResourceManager;

struct Adapter_t;

/**
 * @brief VulkanDevice
 * \ingroup vulkan
 *
 */
struct KDGPU_EXPORT VulkanDevice {
    explicit VulkanDevice(VkDevice _device,
                          uint32_t _apiVersion,
                          VulkanResourceManager *_vulkanResourceManager,
                          const Handle<Adapter_t> &_adapterHandle,
                          const AdapterFeatures &requestedFeatures,
                          bool _isOwned = true) noexcept;

    // Non Copyable
    VulkanDevice(VulkanDevice &) noexcept = delete;
    VulkanDevice &operator=(VulkanDevice &) noexcept = delete;

    // Movable
    VulkanDevice(VulkanDevice &&) noexcept = default;
    VulkanDevice &operator=(VulkanDevice &&) noexcept = default;

    std::vector<QueueDescription> getQueues(VulkanResourceManager *resourceManager,
                                            const std::vector<QueueRequest> &queueRequests,
                                            std::span<AdapterQueueType> queueTypes);

    void waitUntilIdle();

    VmaAllocator getOrCreateExternalMemoryAllocator(ExternalMemoryHandleTypeFlags externalMemoryHandleType);
    VmaAllocator createMemoryAllocator(ExternalMemoryHandleTypeFlags externalMemoryHandleType = ExternalMemoryHandleTypeFlagBits::None) const;

    VkDevice device{ VK_NULL_HANDLE };
    uint32_t apiVersion{};
    AdapterFeatures requestedFeatures{};

    VulkanResourceManager *vulkanResourceManager{ nullptr };
    Handle<Adapter_t> adapterHandle;
    VmaAllocator allocator{ VK_NULL_HANDLE };
    struct MemoryHandleTypeAndAllocator {
        ExternalMemoryHandleTypeFlags externalMemoryHandleType;
        VmaAllocator allocator;
    };
    std::vector<MemoryHandleTypeAndAllocator> externalAllocators;
    std::vector<QueueDescription> queueDescriptions;
    std::vector<VkCommandPool> commandPools; // Indexed by queue type (family)
    std::vector<VkDescriptorPool> descriptorSetPools;
    std::unordered_map<VulkanRenderPassKey, Handle<RenderPass_t>> renderPasses;
    std::unordered_map<VulkanFramebufferKey, Handle<Framebuffer_t>> framebuffers;
    VkQueryPool timestampQueryPool{ VK_NULL_HANDLE };

    PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT{ nullptr };
    PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT{ nullptr };
    PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT{ nullptr };
#if defined(VK_KHR_synchronization2)
    PFN_vkCmdPipelineBarrier2KHR vkCmdPipelineBarrier2{ nullptr };
#endif

#if defined(KDGPU_PLATFORM_WIN32)
    using PFN_vkGetSemaphoreWin32HandleKHR = VkResult(VKAPI_PTR *)(VkDevice, const VkSemaphoreGetWin32HandleInfoKHR *, HANDLE *);
    PFN_vkGetSemaphoreWin32HandleKHR vkGetSemaphoreWin32HandleKHR{ nullptr };

    using PFN_vkGetFenceWin32HandleKHR = VkResult(VKAPI_PTR *)(VkDevice, const VkFenceGetWin32HandleInfoKHR *, HANDLE *);
    PFN_vkGetFenceWin32HandleKHR vkGetFenceWin32HandleKHR{ nullptr };
#endif

#if defined(KDGPU_PLATFORM_LINUX)
    PFN_vkGetSemaphoreFdKHR vkGetSemaphoreFdKHR{ nullptr };
    PFN_vkGetFenceFdKHR vkGetFenceFdKHR{ nullptr };
#endif

    PFN_vkCreateRenderPass2 vkCreateRenderPass2{ nullptr };
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR{ nullptr };
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR{ nullptr };
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR{ nullptr };
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR{ nullptr };
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR{ nullptr };
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR{ nullptr };
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR{ nullptr };
    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR{ nullptr };
    PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT{ nullptr };
    PFN_vkCmdDrawMeshTasksIndirectEXT vkCmdDrawMeshTasksIndirectEXT{ nullptr };

    bool isOwned{ true };
};

} // namespace KDGpu
