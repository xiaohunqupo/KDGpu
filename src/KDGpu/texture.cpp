/*
  This file is part of KDGpu.

  SPDX-FileCopyrightText: 2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>

  SPDX-License-Identifier: MIT

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#include "texture.h"

#include <KDGpu/api/graphics_api_impl.h>
#include <KDGpu/texture_options.h>
#include <KDGpu/command_recorder.h>

namespace KDGpu {

Texture::Texture()
{
}

Texture::Texture(GraphicsApi *api, const Handle<Device_t> &device, const Handle<Texture_t> &texture)
    : m_api(api)
    , m_device(device)
    , m_texture(texture)
{
}

Texture::Texture(GraphicsApi *api, const Handle<Device_t> &device, const TextureOptions &options)
    : Texture(api, device, api->resourceManager()->createTexture(device, options))
{
}

Texture::Texture(Texture &&other) noexcept
{
    m_api = std::exchange(other.m_api, nullptr);
    m_device = std::exchange(other.m_device, {});
    m_texture = std::exchange(other.m_texture, {});
}

Texture &Texture::operator=(Texture &&other) noexcept
{
    if (this != &other) {
        if (isValid())
            m_api->resourceManager()->deleteTexture(handle());

        m_api = std::exchange(other.m_api, nullptr);
        m_device = std::exchange(other.m_device, {});
        m_texture = std::exchange(other.m_texture, {});
    }
    return *this;
}

Texture::~Texture()
{
    if (isValid())
        m_api->resourceManager()->deleteTexture(handle());
}

TextureView Texture::createView(const TextureViewOptions &options) const
{
    auto textureViewHandle = m_api->resourceManager()->createTextureView(m_device, m_texture, options);
    return TextureView(m_api, textureViewHandle);
}

void *Texture::map()
{
    if (!m_mapped && isValid()) {
        auto apiTexture = m_api->resourceManager()->getTexture(m_texture);
        m_mapped = apiTexture->map();
    }
    return m_mapped;
}

void Texture::unmap()
{
    if (!m_mapped)
        return;
    auto apiTexture = m_api->resourceManager()->getTexture(m_texture);
    apiTexture->unmap();
    m_mapped = nullptr;
}

SubresourceLayout Texture::getSubresourceLayout(const TextureSubresource &subresource) const
{
    auto apiTexture = m_api->resourceManager()->getTexture(m_texture);
    return apiTexture->getSubresourceLayout(subresource);
}

bool Texture::generateMipMaps(Device &device, Queue &transferQueue, const Handle<Texture_t> &sourceTexture, const TextureOptions &options, TextureLayout oldLayout, TextureLayout newLayout)
{
    const Adapter *adapter = device.adapter();
    if (!adapter)
        return false;

    if (!adapter->supportsBlitting(options.format, options.tiling))
        return false;

    CommandRecorder commandRecorder = device.createCommandRecorder();

    // Transition source to TransferSrcOptimal
    if (oldLayout != TextureLayout::TransferSrcOptimal)
        commandRecorder.textureMemoryBarrier(TextureMemoryBarrierOptions{
                .srcStages = PipelineStageFlagBit::TransferBit,
                .srcMask = AccessFlagBit::None,
                .dstStages = PipelineStageFlagBit::TransferBit,
                .dstMask = AccessFlagBit::TransferReadBit,
                .oldLayout = oldLayout,
                .newLayout = TextureLayout::TransferSrcOptimal,
                .texture = sourceTexture,
                .range = {
                        .aspectMask = TextureAspectFlagBits::ColorBit,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                },
        });

    // when copying from ourselves, no need to copy from level 0 to 0
    uint32_t startingMipLevel = 1;
    if (sourceTexture != this->handle())
        // when copying from another texture, level 0 also needs to be updated
        startingMipLevel = 0;
    for (uint32_t mipLevel = startingMipLevel; mipLevel < options.mipLevels; ++mipLevel) {

        // Transition miplevel to TransforDstOptimal
        commandRecorder.textureMemoryBarrier(TextureMemoryBarrierOptions{
                .srcStages = PipelineStageFlagBit::TransferBit,
                .srcMask = AccessFlagBit::None,
                .dstStages = PipelineStageFlagBit::TransferBit,
                .dstMask = AccessFlagBit::TransferWriteBit,
                .oldLayout = TextureLayout::Undefined,
                .newLayout = TextureLayout::TransferDstOptimal,
                .texture = m_texture,
                .range = {
                        .aspectMask = TextureAspectFlagBits::ColorBit,
                        .baseMipLevel = mipLevel,
                        .levelCount = 1,
                },
        });

        // Blit main mip level into sub level
        commandRecorder.blitTexture(TextureBlitOptions{
                .srcTexture = sourceTexture,
                .srcLayout = TextureLayout::TransferSrcOptimal,
                .dstTexture = m_texture,
                .dstLayout = TextureLayout::TransferDstOptimal,
                .regions = {
                        {
                                .srcSubresource = {
                                        .aspectMask = TextureAspectFlagBits::ColorBit,
                                        .mipLevel = 0,
                                },
                                .srcOffset = {
                                        .x = 0,
                                        .y = 0,
                                        .z = 0,
                                },
                                .srcExtent = {
                                        .width = options.extent.width,
                                        .height = options.extent.height,
                                        .depth = 1,
                                },
                                .dstSubresource = {
                                        .aspectMask = TextureAspectFlagBits::ColorBit,
                                        .mipLevel = mipLevel,
                                },
                                .dstOffset = {
                                        .x = 0,
                                        .y = 0,
                                        .z = 0,
                                },
                                .dstExtent = {
                                        .width = (options.extent.width >> mipLevel),
                                        .height = (options.extent.height >> mipLevel),
                                        .depth = 1,
                                },
                        },
                },
                .scalingFilter = FilterMode::Linear,
        });

        // Transition miplevel to newLayout
        if (newLayout != TextureLayout::TransferDstOptimal && newLayout != TextureLayout::Undefined)
            commandRecorder.textureMemoryBarrier(TextureMemoryBarrierOptions{
                    .srcStages = PipelineStageFlagBit::TransferBit,
                    .srcMask = AccessFlagBit::TransferWriteBit,
                    .dstStages = PipelineStageFlagBit::TransferBit,
                    .dstMask = AccessFlagBit::TransferReadBit,
                    .oldLayout = TextureLayout::TransferDstOptimal,
                    .newLayout = newLayout,
                    .texture = m_texture,
                    .range = {
                            .aspectMask = TextureAspectFlagBits::ColorBit,
                            .baseMipLevel = mipLevel,
                            .levelCount = 1,
                    },
            });
    }

    // Transition source texture to newLayout
    if (newLayout != TextureLayout::TransferSrcOptimal && newLayout != TextureLayout::Undefined)
        commandRecorder.textureMemoryBarrier(TextureMemoryBarrierOptions{
                .srcStages = PipelineStageFlagBit::TransferBit,
                .srcMask = AccessFlagBit::TransferReadBit,
                .dstStages = PipelineStageFlagBit::TransferBit,
                .dstMask = AccessFlagBit::None,
                .oldLayout = TextureLayout::TransferSrcOptimal,
                .newLayout = newLayout,
                .texture = sourceTexture,
                .range = {
                        .aspectMask = TextureAspectFlagBits::ColorBit,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                },
        });

    CommandBuffer commandBuffer = commandRecorder.finish();

    transferQueue.submit(SubmitOptions{
            .commandBuffers = { commandBuffer },
    });

    transferQueue.waitUntilIdle();
    return true;
}

bool Texture::generateMipMaps(Device &device, Queue &transferQueue, const TextureOptions &options, TextureLayout oldLayout, TextureLayout newLayout)
{
    return generateMipMaps(device, transferQueue, handle(), options, oldLayout, newLayout);
}

MemoryHandle Texture::externalMemoryHandle() const
{
    auto *apiTexture = m_api->resourceManager()->getTexture(m_texture);
    return apiTexture->externalMemoryHandle();
}

void Texture::hostLayoutTransition(const HostLayoutTransition &transition)
{
    auto *apiTexture = m_api->resourceManager()->getTexture(m_texture);
    apiTexture->hostLayoutTransition(transition);
}

void Texture::copyHostMemoryToTexture(const HostMemoryToTextureCopy &copy)
{
    auto *apiTexture = m_api->resourceManager()->getTexture(m_texture);
    apiTexture->copyHostMemoryToTexture(copy);
}

void Texture::copyTextureToHostMemory(const TextureToHostMemoryCopy &copy)
{
    auto *apiTexture = m_api->resourceManager()->getTexture(m_texture);
    apiTexture->copyTextureToHostMemory(copy);
}

void Texture::copyTextureToTextureHost(const TextureToTextureCopyHost &copy)
{
    auto *apiTexture = m_api->resourceManager()->getTexture(m_texture);
    apiTexture->copyTextureToTextureHost(copy);
}

uint64_t Texture::drmFormatModifier() const
{
    auto *apiTexture = m_api->resourceManager()->getTexture(m_texture);
    return apiTexture->drmFormatModifier();
}

bool operator==(const Texture &a, const Texture &b)
{
    return a.m_api == b.m_api && a.m_device == b.m_device && a.m_texture == b.m_texture;
}

bool operator!=(const Texture &a, const Texture &b)
{
    return !(a == b);
}

} // namespace KDGpu
