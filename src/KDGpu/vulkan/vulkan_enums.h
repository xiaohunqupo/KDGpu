/*
  This file is part of KDGpu.

  SPDX-FileCopyrightText: 2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>

  SPDX-License-Identifier: MIT

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#pragma once

#include <KDGpu/gpu_core.h>
#include <KDGpu/kdgpu_export.h>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace KDGpu {

AdapterDeviceType vkPhysicalDeviceTypeToAdapterDeviceType(VkPhysicalDeviceType deviceType);

KDGPU_EXPORT Format vkFormatToFormat(VkFormat format);
VkFormat formatToVkFormat(Format format);

VkSampleCountFlagBits sampleCountFlagBitsToVkSampleFlagBits(SampleCountFlagBits samples);
SampleCountFlagBits vkSampleCountFlagBitsToSampleFlagBits(VkSampleCountFlagBits samples);

ColorSpace vkColorSpaceKHRToColorSpace(VkColorSpaceKHR colorSpace);
VkColorSpaceKHR colorSpaceToVkColorSpaceKHR(ColorSpace colorSpace);

PresentMode vkPresentModeKHRToPresentMode(VkPresentModeKHR presentMode);
VkPresentModeKHR presentModeToVkPresentModeKHR(PresentMode presentMode);

SurfaceTransformFlagBits vkSurfaceTransformFlagBitsKHRToSurfaceTransformFlagBits(VkSurfaceTransformFlagBitsKHR transform);
VkSurfaceTransformFlagBitsKHR surfaceTransformFlagBitsToVkSurfaceTransformFlagBitsKHR(SurfaceTransformFlagBits transform);

CompositeAlphaFlagBits vkCompositeAlphaFlagBitsKHRToCompositeAlphaFlagBits(VkCompositeAlphaFlagBitsKHR compositeAlpha);
VkCompositeAlphaFlagBitsKHR compositeAlphaFlagBitsToVkCompositeAlphaFlagBitsKHR(CompositeAlphaFlagBits compositeAlpha);

SharingMode vkSharingModeToSharingMode(VkSharingMode sharingMode);
VkSharingMode sharingModeToVkSharingMode(SharingMode sharingMode);

VkImageType textureTypeToVkImageType(TextureType textureType);
TextureType vkImageTypeToTextureType(VkImageType textureType);

VkImageTiling textureTilingToVkImageTiling(TextureTiling tiling);
TextureTiling vkImageTilingToTextureTiling(VkImageTiling tiling);

VkImageLayout textureLayoutToVkImageLayout(TextureLayout layout);
TextureLayout vkImageLayoutToTextureLayout(VkImageLayout layout);

ViewType vkImageViewTypeToViewType(VkImageViewType viewType);
VkImageViewType viewTypeToVkImageViewType(ViewType viewType);

MemoryUsage vmaMemoryUsageToMemoryUsage(VmaMemoryUsage memoryUsage);
VmaMemoryUsage memoryUsageToVmaMemoryUsage(MemoryUsage memoryUsage);

VkDescriptorType resourceBindingTypeToVkDescriptorType(ResourceBindingType type);
ResourceBindingType vkDescriptorTypeToVkDescriptorType(VkDescriptorType type);

VkShaderStageFlagBits shaderStageFlagBitsToVkShaderStageFlagBits(ShaderStageFlagBits stage);
ShaderStageFlagBits vkShaderStageFlagBitsToShaderStageFlagBits(VkShaderStageFlagBits stage);

VkVertexInputRate vertexRateToVkVertexInputRate(VertexRate rate);
VertexRate vkVertexInputRateToVertexRate(VkVertexInputRate rate);

VkPrimitiveTopology primitiveTopologyToVkPrimitiveTopology(PrimitiveTopology topology);
PrimitiveTopology vkPrimitiveTopologyToPrimitiveTopology(VkPrimitiveTopology topology);

VkPolygonMode polygonModeToVkPolygonMode(PolygonMode mode);
PolygonMode vkPolygonModeToPolygonMode(VkPolygonMode mode);

VkFrontFace frontFaceToVkFrontFace(FrontFace face);
FrontFace vkFrontFaceToFrontFace(VkFrontFace face);

VkCompareOp compareOperationToVkCompareOp(CompareOperation op);
CompareOperation vkCompareOpToCompareOperation(VkCompareOp op);

VkStencilOp stencilOperationToVkStencilOp(StencilOperation op);
StencilOperation vkStencilOpToStencilOperation(StencilOperation op);

VkBlendFactor blendFactorToVkBlendFactor(BlendFactor factor);
BlendFactor vkBlendFactorToBlendFactor(VkBlendFactor factor);

VkBlendOp blendOperationToVkBlendOp(BlendOperation op);
BlendOperation vkBlendOpToBlendOperation(VkBlendOp op);

VkAttachmentLoadOp attachmentLoadOperationToVkAttachmentLoadOp(AttachmentLoadOperation op);
AttachmentLoadOperation vkAttachmentLoadOperationToAttachmentLoadOperation(VkAttachmentLoadOp op);

VkAttachmentStoreOp attachmentStoreOperationToVkAttachmentStoreOp(AttachmentStoreOperation op);
AttachmentStoreOperation vkAttachmentStoreOperationToAttachmentStoreOperation(VkAttachmentStoreOp op);

VkFilter filterModeToVkFilterMode(FilterMode mode);
VkSamplerMipmapMode mipMapFilterModeToVkSamplerMipmapMode(MipmapFilterMode mode);
VkSamplerAddressMode addressModeToVkSamplerAddressMode(AddressMode mode);

VkAccessFlagBits accessFlagsToVkAccessFlagBits(AccessFlags accessFlags);
VkPipelineStageFlagBits pipelineStageFlagsToVkPipelineStageFlagBits(PipelineStageFlags pipelineFlags);

#if defined(VK_KHR_synchronization2)
VkAccessFlagBits2KHR accessFlagsToVkAccessFlagBits2(AccessFlags accessFlags);
VkPipelineStageFlagBits2KHR pipelineStageFlagsToVkPipelineStageFlagBits2(PipelineStageFlags pipelineFlags);
#endif

VkIndexType indexTypeToVkIndexType(IndexType type);
IndexType vkIndexTypeToIndexType(VkIndexType type);

VkCommandBufferLevel commandBufferLevelToVkCommandBufferLevel(CommandBufferLevel level);

ExternalSemaphoreHandleTypeFlags vkExternalSemaphoreHandleTypeToExternalSemaphoreHandleType(VkExternalSemaphoreHandleTypeFlagBits handleFlags);
VkExternalSemaphoreHandleTypeFlagBits externalSemaphoreHandleTypeToVkExternalSemaphoreHandleType(ExternalSemaphoreHandleTypeFlags handleFlags);

ExternalMemoryHandleTypeFlags vkExternalMemoryHandleTypeToExternalMemoryHandleType(VkExternalMemoryHandleTypeFlagBits handleFlags);
VkExternalMemoryHandleTypeFlagBits externalMemoryHandleTypeToVkExternalMemoryHandleType(ExternalMemoryHandleTypeFlags handleFlags);

ExternalFenceHandleTypeFlags vkExternalFenceHandleTypeToExternalFenceHandleType(VkExternalFenceHandleTypeFlagBits handleFlags);
VkExternalFenceHandleTypeFlagBits externalFenceHandleTypeToVkExternalFenceHandleType(ExternalFenceHandleTypeFlags handleFlags);

ResolveModeFlagBits vkResolveModeToResolveMode(VkResolveModeFlags resolveFlags);
VkResolveModeFlagBits resolveModeToVkResolveMode(ResolveModeFlagBits resolveFlags);

ResolveModeFlags vkResolveModesToResolveModes(VkResolveModeFlags resolveFlags);
VkResolveModeFlags resolveModesToVkResolveModes(ResolveModeFlags resolveFlags);

VkStencilFaceFlagBits stencilFaceToVkStencilFace(StencilFaceFlags flags);

VkDynamicState dynamicStateToVkDynamicState(DynamicState state);

VkDescriptorBindingFlags resourceBindingFlagsToVkDescriptorBindingFlags(ResourceBindingFlags flags);

VkBuildAccelerationStructureModeKHR accelerationStructureModeToVkStructureMode(BuildAccelerationStructureMode mode);

VkAccelerationStructureTypeKHR accelerationStructureTypeToVkAccelerationStructureType(AccelerationStructureType type);

VkGeometryInstanceFlagsKHR geometryInstanceFlagsToVkGeometryInstanceFlags(GeometryInstanceFlags flags);

VkRayTracingShaderGroupTypeKHR rayTracingShaderGroupTypeToVkRayTracingShaderGroupType(RayTracingShaderGroupType type);

VkBuildAccelerationStructureFlagsKHR accelerationStructureFlagsToVkBuildAccelerationStructureFlags(AccelerationStructureFlags flags);

VkDependencyFlags dependencyFlagsToVkDependencyFlags(DependencyFlags flags);

VkImageAspectFlags textureAspectFlagsToVkImageAspectFlags(TextureAspectFlags flags);

} // namespace KDGpu
