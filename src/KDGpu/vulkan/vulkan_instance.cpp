/*
  This file is part of KDGpu.

  SPDX-FileCopyrightText: 2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>

  SPDX-License-Identifier: MIT

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#include "vulkan_instance.h"

#include <KDGpu/vulkan/vulkan_adapter.h>
#include <KDGpu/vulkan/vulkan_resource_manager.h>

#include <KDGpu/utils/logging.h>

#if defined(KDGPU_PLATFORM_WIN32)
// Avoid having to define VK_USE_PLATFORM_WIN32_KHR which would result in windows.h being included when vulkan.h is included
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#if defined(KDGPU_PLATFORM_LINUX)
#include <vulkan/vulkan_xcb.h>
#include <vulkan/vulkan_wayland.h>
#endif

#if defined(KDGPU_PLATFORM_APPLE)
extern VkSurfaceKHR createVulkanSurface(VkInstance instance, const KDGpu::SurfaceOptions &options);
#endif

namespace KDGpu {

VulkanInstance::VulkanInstance(VulkanResourceManager *_vulkanResourceManager, VkInstance _instance, bool _isOwned) noexcept
    : vulkanResourceManager(_vulkanResourceManager)
    , instance(_instance)
    , isOwned(_isOwned)
{
#if defined(VK_KHR_external_memory_fd)
    vkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)vkGetInstanceProcAddr(instance, "vkGetMemoryFdKHR");
#endif

#if defined(VK_EXT_image_drm_format_modifier)
    vkGetImageDrmFormatModifierPropertiesEXT = (PFN_vkGetImageDrmFormatModifierPropertiesEXT)vkGetInstanceProcAddr(instance, "vkGetImageDrmFormatModifierPropertiesEXT");
#endif

#if defined(VK_KHR_external_memory_win32)
    vkGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)vkGetInstanceProcAddr(instance, "vkGetMemoryWin32HandleKHR");
#endif
}

std::vector<Extension> VulkanInstance::extensions() const
{
    return vulkanResourceManager->getInstanceExtensions();
}

std::vector<Handle<Adapter_t>> VulkanInstance::queryAdapters(const Handle<Instance_t> &instanceHandle)
{
    // Query the physical devices from the instance
    uint32_t adapterCount = 0;
    vkEnumeratePhysicalDevices(instance, &adapterCount, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(adapterCount);
    vkEnumeratePhysicalDevices(instance, &adapterCount, physicalDevices.data());

    // Store the resulting physical devices in the resource manager so that
    // the Adapters can access them later, and create the Adapters.
    std::vector<Handle<Adapter_t>> adapterHandles;
    adapterHandles.reserve(adapterCount);
    for (uint32_t adapterIndex = 0; adapterIndex < adapterCount; ++adapterIndex) {
        VulkanAdapter vulkanAdapter{ physicalDevices[adapterIndex], vulkanResourceManager, instanceHandle };
        adapterHandles.emplace_back(vulkanResourceManager->insertAdapter(vulkanAdapter));
        m_physicalDeviceToHandle.emplace(physicalDevices[adapterIndex], adapterHandles.back());
    }

    return adapterHandles;
}

std::vector<AdapterGroup> VulkanInstance::queryAdapterGroups()
{
    uint32_t adapterGroupCount = 0;
    vkEnumeratePhysicalDeviceGroups(instance, &adapterGroupCount, nullptr);

    std::vector<VkPhysicalDeviceGroupProperties> physicalDeviceGroups(adapterGroupCount);
    for (VkPhysicalDeviceGroupProperties &groupProps : physicalDeviceGroups)
        groupProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES;
    vkEnumeratePhysicalDeviceGroups(instance, &adapterGroupCount, physicalDeviceGroups.data());

    std::vector<AdapterGroup> adapterGroups;
    adapterGroups.reserve(adapterGroupCount);
    for (uint32_t adapterGroupIndex = 0; adapterGroupIndex < adapterGroupCount; ++adapterGroupIndex) {
        const VkPhysicalDeviceGroupProperties &deviceGroupProps = physicalDeviceGroups[adapterGroupIndex];
        AdapterGroup adapterGroup{
            .supportsSubsetAllocations = static_cast<bool>(deviceGroupProps.subsetAllocation),
        };

        std::vector<Handle<Adapter_t>> adpterHandles;
        adapterGroup.adapters.reserve(deviceGroupProps.physicalDeviceCount);
        for (size_t i = 0; i < deviceGroupProps.physicalDeviceCount; ++i) {
            const VkPhysicalDevice device = deviceGroupProps.physicalDevices[i];
            const auto it = m_physicalDeviceToHandle.find(device);
            assert(it != m_physicalDeviceToHandle.end());
            adapterGroup.adapters.push_back(it->second);
        }

        adapterGroups.emplace_back(adapterGroup);
    }

    return adapterGroups;
}

Handle<Surface_t> VulkanInstance::createSurface(const SurfaceOptions &options)
{
    VkSurfaceKHR vkSurface{ VK_NULL_HANDLE };
#if defined(KDGPU_PLATFORM_WIN32)
#if defined(VK_KHR_win32_surface)
    PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR{ nullptr };
    vkCreateWin32SurfaceKHR = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR"));
    if (!vkCreateWin32SurfaceKHR)
        return {};

    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hinstance = GetModuleHandle(nullptr);
    createInfo.hwnd = options.hWnd;

    if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &vkSurface) != VK_SUCCESS)
#endif
        return {};
#endif

#if defined(KDGPU_PLATFORM_LINUX)
    if (options.connection != nullptr) {
#if defined(VK_KHR_xcb_surface)
        PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR{ nullptr };
        vkCreateXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateXcbSurfaceKHR");
        if (!vkCreateXcbSurfaceKHR)
            return {};

        VkXcbSurfaceCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        createInfo.connection = options.connection;
        createInfo.window = options.window;

        if (vkCreateXcbSurfaceKHR(instance, &createInfo, nullptr, &vkSurface) != VK_SUCCESS)
#endif
            return {};
    } else if (options.display != nullptr) {
#if defined(VK_KHR_wayland_surface)
        PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR{ nullptr };
        vkCreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWaylandSurfaceKHR");
        if (!vkCreateWaylandSurfaceKHR)
            return {};

        VkWaylandSurfaceCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        createInfo.display = options.display;
        createInfo.surface = options.surface;

        if (vkCreateWaylandSurfaceKHR(instance, &createInfo, nullptr, &vkSurface) != VK_SUCCESS)
#endif
            return {};
    }
#endif

#if defined(KDGPU_PLATFORM_APPLE)
    vkSurface = createVulkanSurface(instance, options);
#endif

#if defined(KDGPU_PLATFORM_ANDROID)
    const VkAndroidSurfaceCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .window = options.window
    };

    if (vkCreateAndroidSurfaceKHR(instance, &createInfo, nullptr, &vkSurface) != VK_SUCCESS)
        return {};
#endif

    VulkanSurface vulkanSurface(vkSurface, instance);
    auto surfaceHandle = vulkanResourceManager->insertSurface(vulkanSurface);
    return surfaceHandle;
}

Handle<Surface_t> VulkanInstance::createSurface(VkSurfaceKHR vkSurface)
{
    VulkanSurface vulkanSurface(vkSurface, instance, false);
    auto surfaceHandle = vulkanResourceManager->insertSurface(vulkanSurface);
    return surfaceHandle;
}

} // namespace KDGpu
