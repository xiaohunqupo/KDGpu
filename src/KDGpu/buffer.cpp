/*
  This file is part of KDGpu.

  SPDX-FileCopyrightText: 2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>

  SPDX-License-Identifier: MIT

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#include "buffer.h"

#include <KDGpu/api/graphics_api_impl.h>
#include <KDGpu/buffer_options.h>

namespace KDGpu {

Buffer::Buffer() = default;

Buffer::Buffer(GraphicsApi *api, const Handle<Device_t> &device, const BufferOptions &options, const void *initialData)
    : m_api(api)
    , m_device(device)
    , m_buffer(m_api->resourceManager()->createBuffer(m_device, options, initialData))
{
}

Buffer::Buffer(Buffer &&other) noexcept
{
    m_api = std::exchange(other.m_api, nullptr);
    m_device = std::exchange(other.m_device, {});
    m_buffer = std::exchange(other.m_buffer, {});
}

Buffer &Buffer::operator=(Buffer &&other) noexcept
{
    if (this != &other) {
        if (isValid()) {
            if (m_mapped)
                unmap();
            m_api->resourceManager()->deleteBuffer(handle());
        }

        m_api = std::exchange(other.m_api, nullptr);
        m_device = std::exchange(other.m_device, {});
        m_buffer = std::exchange(other.m_buffer, {});
    }
    return *this;
}

Buffer::~Buffer()
{
    if (m_mapped)
        unmap();

    if (isValid())
        m_api->resourceManager()->deleteBuffer(handle());
}

void *Buffer::map()
{
    if (!m_mapped && isValid()) {
        auto apiBuffer = m_api->resourceManager()->getBuffer(m_buffer);
        m_mapped = apiBuffer->map();
    }
    return m_mapped;
}

void Buffer::unmap()
{
    if (!m_mapped)
        return;
    auto apiBuffer = m_api->resourceManager()->getBuffer(m_buffer);
    apiBuffer->unmap();
    m_mapped = nullptr;
}

void Buffer::invalidate()
{
    auto apiBuffer = m_api->resourceManager()->getBuffer(m_buffer);
    apiBuffer->invalidate();
}

void Buffer::flush()
{
    auto apiBuffer = m_api->resourceManager()->getBuffer(m_buffer);
    apiBuffer->flush();
}

MemoryHandle Buffer::externalMemoryHandle() const
{
    auto apiBuffer = m_api->resourceManager()->getBuffer(m_buffer);
    return apiBuffer->externalMemoryHandle();
}

BufferDeviceAddress Buffer::bufferDeviceAddress() const
{
    auto apiBuffer = m_api->resourceManager()->getBuffer(m_buffer);
    return apiBuffer->bufferDeviceAddress();
}

bool operator==(const Buffer &a, const Buffer &b)
{
    return a.m_api == b.m_api && a.m_device == b.m_device && a.m_buffer == b.m_buffer && a.m_mapped == b.m_mapped;
}

bool operator!=(const Buffer &a, const Buffer &b)
{
    return !(a == b);
}

} // namespace KDGpu
