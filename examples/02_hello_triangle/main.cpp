#include <toy_renderer/instance.h>
#include <toy_renderer/formatters.h>
#include <toy_renderer/gpu_core.h>
#include <toy_renderer/vulkan/vulkan_graphics_api.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <iostream>
#include <map>
#include <memory>
#include <span>
#include <vector>

using namespace ToyRenderer;

int main()
{
    std::unique_ptr<GraphicsApi> api = std::make_unique<VulkanGraphicsApi>();
    // OR
    // std::unique_ptr<GraphicsApi> api = std::make_unique<MetalGraphicsApi>();
    // std::unique_ptr<GraphicsApi> api = std::make_unique<DX12GraphicsApi>();

    // Request an instance of the api with whatever layers and extensions we wish to request.
    InstanceOptions instanceOptions = {
        .applicationName = "02_hello_triangle",
        .applicationVersion = SERENITY_MAKE_API_VERSION(0, 1, 0, 0)
    };
    auto instance = api->createInstance(instanceOptions);

    // Enumerate the adapters (physical devices) and select one to use. Here we look for
    // a discrete GPU. In a real app, we could fallback to an integrated one.
    Adapter selectedAdapter;
    auto adapters = instance.adapters();
    for (auto &adapter : adapters) {
        const auto properties = adapter.properties();
        spdlog::critical("Found device: Name: {}, Type: {}", properties.deviceName, properties.deviceType);

        if (properties.deviceType == AdapterDeviceType::DiscreteGpu) {
            selectedAdapter = adapter;
            break;
        }
    }

    if (!selectedAdapter.isValid()) {
        spdlog::critical("Unable to find a discrete GPU. Aborting...");
        return -1;
    }

    return 0;
}
