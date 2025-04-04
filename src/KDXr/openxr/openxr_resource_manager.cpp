/*
  This file is part of KDXr.

  SPDX-FileCopyrightText: 2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>

  SPDX-License-Identifier: MIT

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#include "openxr_resource_manager.h"

#include <KDXr/config.h>
#include <KDXr/instance.h>
#include <KDXr/openxr/openxr_enums.h>
#include <KDXr/utils/logging.h>

#include <KDGpu/vulkan/vulkan_graphics_api.h>

// TODO: Abstract away the choice of graphics API
#include <vulkan/vulkan.h>

#include <openxr/openxr.h>
#define XR_USE_GRAPHICS_API_VULKAN
#include <openxr/openxr_platform.h>

#include <cassert>
#include <stdexcept>

namespace {

static XRAPI_ATTR XrBool32 XRAPI_CALL debugCallback(
        XrDebugUtilsMessageSeverityFlagsEXT messageSeverity,
        XrDebugUtilsMessageTypeFlagsEXT messageType,
        const XrDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData)
{
    switch (messageSeverity) {
    case XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        SPDLOG_LOGGER_DEBUG(KDXr::Logger::logger(), "KDXr message: {}", pCallbackData->message);
        break;
    case XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        SPDLOG_LOGGER_INFO(KDXr::Logger::logger(), "KDXr message: {}", pCallbackData->message);
        break;
    case XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        SPDLOG_LOGGER_WARN(KDXr::Logger::logger(), "KDXr message: {}", pCallbackData->message);
        break;
    case XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        SPDLOG_LOGGER_ERROR(KDXr::Logger::logger(), "KDXr message: {}", pCallbackData->message);
        break;
    default:
        SPDLOG_LOGGER_TRACE(KDXr::Logger::logger(), "KDXr message: {}", pCallbackData->message);
        break;
    }

    return XR_FALSE;
}

bool findExtension(const std::vector<KDXr::Extension> &extensions, const std::string_view &name)
{
    const auto it = std::find_if(begin(extensions), end(extensions), [name](const KDXr::Extension &ext) { return ext.name == name; });
    return it != std::end(extensions);
};

} // namespace

namespace KDXr {

OpenXrResourceManager::OpenXrResourceManager()
{
}

OpenXrResourceManager::~OpenXrResourceManager()
{
}

std::vector<ApiLayer> OpenXrResourceManager::availableApiLayers() const
{
    uint32_t apiLayerCount = 0;
    std::vector<XrApiLayerProperties> apiLayerProperties;
    if (xrEnumerateApiLayerProperties(0, &apiLayerCount, nullptr) != XR_SUCCESS) {
        SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to enumerate ApiLayerProperties.");
    }
    apiLayerProperties.resize(apiLayerCount, { XR_TYPE_API_LAYER_PROPERTIES });
    if (xrEnumerateApiLayerProperties(apiLayerCount, &apiLayerCount, apiLayerProperties.data()) != XR_SUCCESS) {
        SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to enumerate ApiLayerProperties.");
    }

    std::vector<ApiLayer> layers;
    layers.reserve(apiLayerCount);
    for (const auto &apiLayerProperty : apiLayerProperties) {
        layers.emplace_back(ApiLayer{ .name = apiLayerProperty.layerName,
                                      .description = apiLayerProperty.description,
                                      .specVersion = apiLayerProperty.specVersion,
                                      .layerVersion = apiLayerProperty.layerVersion });
    }

    return layers;
}

std::vector<Extension> OpenXrResourceManager::availableInstanceExtensions() const
{
    uint32_t extensionCount = 0;
    std::vector<XrExtensionProperties> extensionProperties;
    if (xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr) != XR_SUCCESS) {
        SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to enumerate InstanceExtensionProperties.");
    }
    extensionProperties.resize(extensionCount, { XR_TYPE_EXTENSION_PROPERTIES });
    if (xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, extensionProperties.data()) != XR_SUCCESS) {
        SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to enumerate InstanceExtensionProperties.");
    }

    std::vector<Extension> extensions;
    extensions.reserve(extensionCount);
    for (const auto &extensionProperty : extensionProperties) {
        extensions.emplace_back(Extension{ .name = extensionProperty.extensionName,
                                           .extensionVersion = extensionProperty.extensionVersion });
    }

    return extensions;
}

KDGpu::Handle<Instance_t> OpenXrResourceManager::createInstance(const InstanceOptions &options)
{
    XrApplicationInfo xrApplicationInfo = {};
    strncpy(xrApplicationInfo.applicationName, options.applicationName.data(), XR_MAX_APPLICATION_NAME_SIZE);
    xrApplicationInfo.applicationVersion = 1;
    strncpy(xrApplicationInfo.engineName, "KDXr Engine", XR_MAX_ENGINE_NAME_SIZE);
    xrApplicationInfo.engineVersion = 1;
    xrApplicationInfo.apiVersion = XR_CURRENT_API_VERSION;

    // Check the requested API layers against the ones enumerated from OpenXR. If found add it to the active api Layers.
    auto apiLayers = availableApiLayers();
    std::vector<const char *> xrActiveApiLayers{}; // To pass to xrCreateInstance
    std::vector<ApiLayer> openXrActiveApiLayers; // To store in OpenXrInstance
    for (auto &requestedLayer : options.layers) {
        bool found = false;
        for (auto &layerProperty : apiLayers) {
            if (requestedLayer != layerProperty.name) {
                continue;
            } else {
                xrActiveApiLayers.push_back(requestedLayer.c_str());
                openXrActiveApiLayers.push_back(layerProperty);
                found = true;
                break;
            }
        }
        if (!found) {
            SPDLOG_LOGGER_WARN(Logger::logger(), "Failed to find requested api layer: {}", requestedLayer);
        }
    }

    // Query and check instance extensions
    auto instanceExtensions = availableInstanceExtensions();

    // If the runtime supports the XR_FB_composition_layer_depth_test extension, add it to the list of requested extensions.
    // This allows the OpenXR compositor to correctly depth test the compositor layers.
    auto requestedInstanceExtensions = options.extensions;
    if (findExtension(instanceExtensions, XR_FB_COMPOSITION_LAYER_DEPTH_TEST_EXTENSION_NAME))
        requestedInstanceExtensions.push_back(XR_FB_COMPOSITION_LAYER_DEPTH_TEST_EXTENSION_NAME);

    std::vector<const char *> xrActiveInstanceExtensions; // To pass to xrCreateInstance
    std::vector<Extension> openXrActiveExtensions; // To store in OpenXrInstance
    for (auto &requestedInstanceExtension : requestedInstanceExtensions) {
        bool found = false;
        for (auto &extensionProperty : instanceExtensions) {
            if (requestedInstanceExtension != extensionProperty.name) {
                continue;
            } else {
                xrActiveInstanceExtensions.push_back(requestedInstanceExtension.c_str());
                openXrActiveExtensions.push_back(extensionProperty);
                found = true;
                break;
            }
        }
        if (!found) {
            SPDLOG_LOGGER_WARN(Logger::logger(), "Failed to find requested instance extension: {}", requestedInstanceExtension);
        }
    }

    // Create the XR Instance
    XrInstanceCreateInfo instanceCI{ XR_TYPE_INSTANCE_CREATE_INFO };
    instanceCI.createFlags = 0;
    instanceCI.applicationInfo = xrApplicationInfo;
    instanceCI.enabledApiLayerCount = static_cast<uint32_t>(xrActiveApiLayers.size());
    instanceCI.enabledApiLayerNames = xrActiveApiLayers.data();
    instanceCI.enabledExtensionCount = static_cast<uint32_t>(xrActiveInstanceExtensions.size());
    instanceCI.enabledExtensionNames = xrActiveInstanceExtensions.data();

    XrInstance xrInstance{ XR_NULL_HANDLE };
    if (xrCreateInstance(&instanceCI, &xrInstance) != XR_SUCCESS) {

        // As a fallback, try to create a version 1.0 instance
        // This may be required for the SteamVR OpenXR runtime
        instanceCI.applicationInfo.apiVersion = XR_API_VERSION_1_0;

        if (xrCreateInstance(&instanceCI, &xrInstance) != XR_SUCCESS) {
            throw std::runtime_error("Failed to create OpenXR Instance.");
        }
    }

    OpenXrInstance openXrInstance(this, xrInstance, openXrActiveApiLayers, openXrActiveExtensions);

    // Create the debug logger
    bool foundDebugUtilsExtension = false;
    for (const auto &extension : xrActiveInstanceExtensions) {
        if (strcmp(extension, XR_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
            foundDebugUtilsExtension = true;
            break;
        }
    }

    if (foundDebugUtilsExtension == true) {
        XrDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo{ XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
        debugUtilsMessengerCreateInfo.messageSeverities = XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugUtilsMessengerCreateInfo.messageTypes = XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
        debugUtilsMessengerCreateInfo.userCallback = (PFN_xrDebugUtilsMessengerCallbackEXT)debugCallback;
        debugUtilsMessengerCreateInfo.userData = nullptr;

        // Load xrCreateDebugUtilsMessengerEXT() function pointer as it is not default loaded by the OpenXR loader.
        PFN_xrCreateDebugUtilsMessengerEXT xrCreateDebugUtilsMessengerEXT;
        if (xrGetInstanceProcAddr(openXrInstance.instance, "xrCreateDebugUtilsMessengerEXT", (PFN_xrVoidFunction *)&xrCreateDebugUtilsMessengerEXT) != XR_SUCCESS) {
            SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to get InstanceProcAddr.");
        }

        // Finally create and return the XrDebugUtilsMessengerEXT.
        if (xrCreateDebugUtilsMessengerEXT != nullptr) {
            if (xrCreateDebugUtilsMessengerEXT(openXrInstance.instance, &debugUtilsMessengerCreateInfo, &openXrInstance.debugMessenger) != XR_SUCCESS) {
                SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to create DebugUtilsMessenger.");
            }
        }
    }

    auto h = m_instances.emplace(openXrInstance);
    return h;
}

void OpenXrResourceManager::deleteInstance(const KDGpu::Handle<Instance_t> &handle)
{
    OpenXrInstance *openXrInstance = m_instances.get(handle);

    // Only destroy instances that we have allocated
    if (openXrInstance->isOwned) {
        // Destroy debug logger if we have one
        if (openXrInstance->debugMessenger) {
            // Load xrDestroyDebugUtilsMessengerEXT() function pointer as it is not default loaded by the OpenXR loader.
            PFN_xrDestroyDebugUtilsMessengerEXT xrDestroyDebugUtilsMessengerEXT;
            if (xrGetInstanceProcAddr(openXrInstance->instance, "xrDestroyDebugUtilsMessengerEXT", (PFN_xrVoidFunction *)&xrDestroyDebugUtilsMessengerEXT) != XR_SUCCESS) {
                SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to get InstanceProcAddr.");
            }

            // Destroy the provided XrDebugUtilsMessengerEXT.
            if (xrDestroyDebugUtilsMessengerEXT != nullptr) {
                if (xrDestroyDebugUtilsMessengerEXT(openXrInstance->debugMessenger) != XR_SUCCESS) {
                    SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to destroy DebugUtilsMessenger.");
                }
            }
        }

        xrDestroyInstance(openXrInstance->instance);
    }

    m_instances.remove(handle);
}

OpenXrInstance *OpenXrResourceManager::getInstance(const KDGpu::Handle<Instance_t> &handle) const
{
    return m_instances.get(handle);
}

KDGpu::Handle<System_t> OpenXrResourceManager::insertSystem(const OpenXrSystem &openXrSystem)
{
    return m_systems.emplace(openXrSystem);
}

void OpenXrResourceManager::removeSystem(const KDGpu::Handle<System_t> &handle)
{
    m_systems.remove(handle);
}

OpenXrSystem *OpenXrResourceManager::getSystem(const KDGpu::Handle<System_t> &handle) const
{
    return m_systems.get(handle);
}

KDGpu::Handle<Session_t> OpenXrResourceManager::createSession(const KDGpu::Handle<System_t> &systemHandle, const SessionOptions &options)
{
    OpenXrSystem *openXrSystem = m_systems.get(systemHandle);
    assert(openXrSystem);
    OpenXrInstance *openXrInstance = m_instances.get(openXrSystem->instanceHandle);
    assert(openXrInstance);

    XrSessionCreateInfo sessionCI{ XR_TYPE_SESSION_CREATE_INFO };
    sessionCI.systemId = openXrSystem->system;

    // Determine which graphics API is in use
    if (auto vulkanGraphicsApi = dynamic_cast<KDGpu::VulkanGraphicsApi *>(options.graphicsApi)) {
        // Vulkan is in use
        auto graphicsResourceManager = dynamic_cast<KDGpu::VulkanResourceManager *>(vulkanGraphicsApi->resourceManager());
        assert(graphicsResourceManager);

        auto vulkanDevice = graphicsResourceManager->getDevice(options.device);
        auto vulkanAdapter = graphicsResourceManager->getAdapter(vulkanDevice->adapterHandle);
        auto vulkanInstance = graphicsResourceManager->getInstance(vulkanAdapter->instanceHandle);
        assert(options.queueIndex < vulkanDevice->queueDescriptions.size());
        const auto &queueDescription = vulkanDevice->queueDescriptions[options.queueIndex];

        XrGraphicsBindingVulkanKHR graphicsBinding{ XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR };
        graphicsBinding.instance = vulkanInstance->instance;
        graphicsBinding.physicalDevice = vulkanAdapter->physicalDevice;
        graphicsBinding.device = vulkanDevice->device;
        graphicsBinding.queueFamilyIndex = queueDescription.queueTypeIndex;
        graphicsBinding.queueIndex = options.queueIndex;

        // Set graphics binding on the session create info
        sessionCI.next = &graphicsBinding;
    } else {
        throw std::runtime_error("Only Vulkan is supported at the moment.");
    }

    XrSession xrSession{ XR_NULL_HANDLE };
    if (xrCreateSession(openXrInstance->instance, &sessionCI, &xrSession) != XR_SUCCESS) {
        SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to create OpenXR Session.");
        return {};
    }

    auto h = m_sessions.emplace(OpenXrSession{ this, xrSession, systemHandle, openXrSystem->instanceHandle,
                                               options.graphicsApi, options.device, options.queueIndex });

    // Register the session with the instance so that we can look it up when processing OpenXR events
    openXrInstance->m_sessionToHandle.insert({ xrSession, h });

    return h;
}

void OpenXrResourceManager::deleteSession(const KDGpu::Handle<Session_t> &handle)
{
    // Unregister the session from the instance
    OpenXrSession *openXrSession = m_sessions.get(handle);
    assert(openXrSession);
    OpenXrSystem *openXrSystem = m_systems.get(openXrSession->systemHandle);
    assert(openXrSystem);
    OpenXrInstance *openXrInstance = m_instances.get(openXrSystem->instanceHandle);
    assert(openXrInstance);
    openXrInstance->m_sessionToHandle.erase(openXrSession->session);

    if (xrDestroySession(openXrSession->session) != XR_SUCCESS) {
        SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to destroy OpenXR Session.");
    }
    m_sessions.remove(handle);
}

OpenXrSession *OpenXrResourceManager::getSession(const KDGpu::Handle<Session_t> &handle) const
{
    return m_sessions.get(handle);
}

KDGpu::Handle<ReferenceSpace_t> OpenXrResourceManager::createReferenceSpace(const KDGpu::Handle<Session_t> &sessionHandle, const ReferenceSpaceOptions &options)
{
    OpenXrSession *openXrSession = m_sessions.get(sessionHandle);

    const XrQuaternionf orientation{ options.pose.orientation.x, options.pose.orientation.y, options.pose.orientation.z, options.pose.orientation.w };
    const XrVector3f offset{ options.pose.position.x, options.pose.position.y, options.pose.position.z };
    XrReferenceSpaceCreateInfo referenceSpaceCreateInfo{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    referenceSpaceCreateInfo.referenceSpaceType = static_cast<XrReferenceSpaceType>(options.type); // TODO: Add conversion function
    referenceSpaceCreateInfo.poseInReferenceSpace = { orientation, offset };
    XrSpace xrReferenceSpace{ XR_NULL_HANDLE };
    if (xrCreateReferenceSpace(openXrSession->session, &referenceSpaceCreateInfo, &xrReferenceSpace) != XR_SUCCESS) {
        SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to create OpenXR Reference Space.");
        return {};
    }

    auto h = m_referenceSpaces.emplace(OpenXrReferenceSpace{ this, xrReferenceSpace, sessionHandle, options.type, options.pose });
    return h;
}

KDGpu::Handle<ReferenceSpace_t> OpenXrResourceManager::createReferenceSpace(const KDGpu::Handle<Session_t> &sessionHandle, const ActionSpaceOptions &options)
{
    OpenXrAction *openXrAction = m_actions.get(options.action);
    assert(openXrAction);
    OpenXrSession *openXrSession = m_sessions.get(sessionHandle);
    assert(openXrSession);

    XrPath xrPath{ XR_NULL_PATH };
    if (!options.subactionPath.empty()) {
        OpenXrInstance *openXrInstance = m_instances.get(openXrSession->instanceHandle);
        assert(openXrInstance);
        xrPath = openXrInstance->createXrPath(options.subactionPath);
    }

    const auto &pose = options.poseInActionSpace;
    const XrQuaternionf orientation{ pose.orientation.x, pose.orientation.y, pose.orientation.z, pose.orientation.w };
    const XrVector3f offset{ pose.position.x, pose.position.y, pose.position.z };
    XrActionSpaceCreateInfo actionSpaceCreateInfo{ XR_TYPE_ACTION_SPACE_CREATE_INFO };
    actionSpaceCreateInfo.action = openXrAction->action;
    actionSpaceCreateInfo.poseInActionSpace = { orientation, offset };
    actionSpaceCreateInfo.subactionPath = xrPath;

    XrSpace xrReferenceSpace{ XR_NULL_HANDLE };
    if (xrCreateActionSpace(openXrSession->session, &actionSpaceCreateInfo, &xrReferenceSpace) != XR_SUCCESS) {
        SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to create OpenXR Reference Space.");
        return {};
    }

    auto h = m_referenceSpaces.emplace(OpenXrReferenceSpace{ this, xrReferenceSpace, sessionHandle, options.action, pose });
    return h;
}

void OpenXrResourceManager::deleteReferenceSpace(const KDGpu::Handle<ReferenceSpace_t> &handle)
{
    OpenXrReferenceSpace *openXrReferenceSpace = m_referenceSpaces.get(handle);
    if (xrDestroySpace(openXrReferenceSpace->referenceSpace) != XR_SUCCESS) {
        SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to destroy OpenXR ReferenceSpace.");
    }
    m_referenceSpaces.remove(handle);
}

OpenXrReferenceSpace *OpenXrResourceManager::getReferenceSpace(const KDGpu::Handle<ReferenceSpace_t> &handle) const
{
    return m_referenceSpaces.get(handle);
}

KDGpu::Handle<PassthroughLayer_t> OpenXrResourceManager::createPassthroughLayer(const KDGpu::Handle<Session_t> &sessionHandle, const PassthroughLayerOptions &options)
{
    OpenXrSession *openXrSession = m_sessions.get(sessionHandle);
    assert(openXrSession);
    OpenXrInstance *openXrInstance = m_instances.get(openXrSession->instanceHandle);
    assert(openXrInstance);

    if (m_passthrough == XR_NULL_HANDLE) {
        // Create a singleton passthrough object, if we do not already have one
        XrPassthroughCreateInfoFB passthroughCreateInfo = { XR_TYPE_PASSTHROUGH_CREATE_INFO_FB };
        passthroughCreateInfo.flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB;

        PFN_xrCreatePassthroughFB xrCreatePassthroughFB;
        if (xrGetInstanceProcAddr(openXrInstance->instance, "xrCreatePassthroughFB", (PFN_xrVoidFunction *)&xrCreatePassthroughFB) != XR_SUCCESS) {
            SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to get InstanceProcAddr.");
            return {};
        }

        if (xrCreatePassthroughFB(openXrSession->session, &passthroughCreateInfo, &m_passthrough) != XR_SUCCESS) {
            SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to create OpenXR Passthrough.");
            return {};
        }
    }

    PFN_xrCreatePassthroughLayerFB xrCreatePassthroughLayerFB;
    if (xrGetInstanceProcAddr(openXrInstance->instance, "xrCreatePassthroughLayerFB", (PFN_xrVoidFunction *)&xrCreatePassthroughLayerFB) != XR_SUCCESS) {
        SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to get InstanceProcAddr.");
    }

    XrPassthroughLayerCreateInfoFB passthroughLayerCreateInfo = { XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB };
    passthroughLayerCreateInfo.passthrough = m_passthrough;
    passthroughLayerCreateInfo.purpose = XR_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION_FB;
    passthroughLayerCreateInfo.flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB;

    XrPassthroughLayerFB xrPassthroughLayer{ XR_NULL_HANDLE };
    if ((xrCreatePassthroughLayerFB(openXrSession->session, &passthroughLayerCreateInfo, &xrPassthroughLayer)) != XR_SUCCESS) {
        SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to create OpenXR Passthrough Layer.");
        return {};
    }

    auto h = m_passthroughLayers.emplace(OpenXrPassthroughLayer{
            this,
            xrPassthroughLayer,
            sessionHandle,
            options });
    return h;
}

void OpenXrResourceManager::deletePassthroughLayer(const KDGpu::Handle<PassthroughLayer_t> &handle)
{
    OpenXrPassthroughLayer *openXrPassthroughLayer = m_passthroughLayers.get(handle);
    assert(openXrPassthroughLayer);

    OpenXrSession *openXrSession = m_sessions.get(openXrPassthroughLayer->sessionHandle);
    assert(openXrSession);
    OpenXrInstance *openXrInstance = m_instances.get(openXrSession->instanceHandle);
    assert(openXrInstance);

    PFN_xrDestroyPassthroughLayerFB xrDestroyPassthroughLayerFB;
    if (xrGetInstanceProcAddr(openXrInstance->instance, "xrDestroyPassthroughLayerFB", (PFN_xrVoidFunction *)&xrDestroyPassthroughLayerFB) != XR_SUCCESS) {
        SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to get InstanceProcAddr.");
    }

    if (xrDestroyPassthroughLayerFB(openXrPassthroughLayer->passthroughLayer) != XR_SUCCESS) {
        SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to destroy OpenXR Passthrough Layer.");
    }
    m_passthroughLayers.remove(handle);

    if (m_passthroughLayers.size() == 0 && m_passthrough != XR_NULL_HANDLE) {
        // If all the layers are gone, we no longer need the passthrough object

        PFN_xrDestroyPassthroughFB xrDestroyPassthroughFB;
        if (xrGetInstanceProcAddr(openXrInstance->instance, "xrDestroyPassthroughFB", (PFN_xrVoidFunction *)&xrDestroyPassthroughFB) != XR_SUCCESS) {
            SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to get InstanceProcAddr.");
        }

        if (xrDestroyPassthroughFB(m_passthrough) != XR_SUCCESS) {
            SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to destroy OpenXR Passthrough.");
        }

        m_passthrough = XR_NULL_HANDLE;
    }
}

OpenXrPassthroughLayer *OpenXrResourceManager::getPassthroughLayer(const KDGpu::Handle<PassthroughLayer_t> &handle) const
{
    return m_passthroughLayers.get(handle);
}

KDGpu::Handle<Swapchain_t> OpenXrResourceManager::createSwapchain(const KDGpu::Handle<Session_t> &sessionHandle, const SwapchainOptions &options)
{
    OpenXrSession *openXrSession = m_sessions.get(sessionHandle);

    XrSwapchainCreateInfo swapchainCreateInfo{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
    swapchainCreateInfo.createFlags = 0;
    swapchainCreateInfo.usageFlags = swapchainUsageFlagsToXrSwapchainUsageFlags(options.usage);
    swapchainCreateInfo.format = static_cast<int64_t>(options.format);
    swapchainCreateInfo.sampleCount = options.sampleCount;
    swapchainCreateInfo.width = options.width;
    swapchainCreateInfo.height = options.height;
    swapchainCreateInfo.faceCount = options.faceCount;
    swapchainCreateInfo.arraySize = options.arrayLayers;
    swapchainCreateInfo.mipCount = options.mipLevels;

    XrSwapchain xrSwapchain{ XR_NULL_HANDLE };
    if (xrCreateSwapchain(openXrSession->session, &swapchainCreateInfo, &xrSwapchain) != XR_SUCCESS) {
        SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to create OpenXR Swapchain.");
        return {};
    }

    auto h = m_swapchains.emplace(OpenXrSwapchain{
            this,
            xrSwapchain,
            sessionHandle,
            options });
    return h;
}

void OpenXrResourceManager::deleteSwapchain(const KDGpu::Handle<Swapchain_t> &handle)
{
    OpenXrSwapchain *openXrSwapchain = m_swapchains.get(handle);
    if (xrDestroySwapchain(openXrSwapchain->swapchain) != XR_SUCCESS) {
        SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to destroy OpenXR Swapchain.");
    }
    m_swapchains.remove(handle);
}

OpenXrSwapchain *OpenXrResourceManager::getSwapchain(const KDGpu::Handle<Swapchain_t> &handle) const
{
    return m_swapchains.get(handle);
}

KDGpu::Handle<ActionSet_t> OpenXrResourceManager::createActionSet(const KDGpu::Handle<Instance_t> &instanceHandle, const ActionSetOptions &options)
{
    OpenXrInstance *openXrInstance = m_instances.get(instanceHandle);

    XrActionSetCreateInfo actionSetCreateInfo{ XR_TYPE_ACTION_SET_CREATE_INFO };
    strncpy(actionSetCreateInfo.actionSetName, options.name.data(), XR_MAX_ACTION_SET_NAME_SIZE);
    strncpy(actionSetCreateInfo.localizedActionSetName, options.localizedName.data(), XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE);
    actionSetCreateInfo.priority = options.priority;

    XrActionSet xrActionSet{ XR_NULL_HANDLE };
    if (xrCreateActionSet(openXrInstance->instance, &actionSetCreateInfo, &xrActionSet) != XR_SUCCESS) {
        SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to create OpenXR ActionSet.");
        return {};
    }

    auto h = m_actionSets.emplace(OpenXrActionSet{ this, xrActionSet, instanceHandle });
    return h;
}

void OpenXrResourceManager::deleteActionSet(const KDGpu::Handle<ActionSet_t> &handle)
{
    OpenXrActionSet *openXrActionSet = m_actionSets.get(handle);
    if (xrDestroyActionSet(openXrActionSet->actionSet) != XR_SUCCESS) {
        SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to destroy OpenXR ActionSet.");
    }
    m_actionSets.remove(handle);
}

OpenXrActionSet *OpenXrResourceManager::getActionSet(const KDGpu::Handle<ActionSet_t> &handle) const
{
    return m_actionSets.get(handle);
}

KDGpu::Handle<Action_t> OpenXrResourceManager::createAction(const KDGpu::Handle<ActionSet_t> &actionSetHandle, const ActionOptions &options)
{
    OpenXrActionSet *openXrActionSet = m_actionSets.get(actionSetHandle);
    assert(openXrActionSet);
    OpenXrInstance *openXrInstance = m_instances.get(openXrActionSet->instanceHandle);
    assert(openXrInstance);

    // Subaction paths, e.g. left and right hand. To distinguish the same action performed on different devices.
    std::vector<XrPath> xrSubactionPaths;
    xrSubactionPaths.reserve(options.subactionPaths.size());
    for (const auto &path : options.subactionPaths)
        xrSubactionPaths.push_back(openXrInstance->createXrPath(path));

    XrActionCreateInfo actionCreateInfo{ XR_TYPE_ACTION_CREATE_INFO };
    strncpy(actionCreateInfo.actionName, options.name.data(), XR_MAX_ACTION_NAME_SIZE);
    actionCreateInfo.actionType = actionTypeToXrActionType(options.type);
    actionCreateInfo.countSubactionPaths = static_cast<uint32_t>(xrSubactionPaths.size());
    actionCreateInfo.subactionPaths = xrSubactionPaths.data();
    strncpy(actionCreateInfo.localizedActionName, options.localizedName.data(), XR_MAX_LOCALIZED_ACTION_NAME_SIZE);

    XrAction xrAction{ XR_NULL_HANDLE };
    if (xrCreateAction(openXrActionSet->actionSet, &actionCreateInfo, &xrAction) != XR_SUCCESS) {
        SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to create OpenXR Action.");
        return {};
    }

    auto h = m_actions.emplace(OpenXrAction{ this, xrAction, actionSetHandle });
    return h;
}

void OpenXrResourceManager::deleteAction(const KDGpu::Handle<Action_t> &handle)
{
    OpenXrAction *openXrAction = m_actions.get(handle);
    if (xrDestroyAction(openXrAction->action) != XR_SUCCESS) {
        SPDLOG_LOGGER_CRITICAL(Logger::logger(), "Failed to destroy OpenXR Action.");
    }
    m_actions.remove(handle);
}

OpenXrAction *OpenXrResourceManager::getAction(const KDGpu::Handle<Action_t> &handle) const
{
    return m_actions.get(handle);
}

} // namespace KDXr
