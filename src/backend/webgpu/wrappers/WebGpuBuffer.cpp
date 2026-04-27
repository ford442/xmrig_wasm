/* XMRig
 * Copyright (c) 2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 */

#include "backend/webgpu/wrappers/WebGpuBuffer.h"
#include "backend/webgpu/wrappers/WebGpuDevice.h"

extern "C" {
    int wgpu_create_buffer(int device_id, uint32_t size, int usage);
    void wgpu_destroy_buffer(int device_id, int buffer_id);
    void wgpu_write_buffer(int device_id, int buffer_id, uint32_t offset, const void* data, uint32_t size);
    void wgpu_read_buffer(int device_id, int buffer_id, uint32_t offset, void* dst, uint32_t size);
}

namespace xmrig {

WebGpuBuffer::WebGpuBuffer(const WebGpuDevice &device, uint32_t size, uint32_t usage)
    : m_deviceId(device.id())
    , m_size(size)
{
    m_bufferId = wgpu_create_buffer(m_deviceId, size, static_cast<int>(usage));
}

WebGpuBuffer::~WebGpuBuffer()
{
    if (m_bufferId > 0) {
        wgpu_destroy_buffer(m_deviceId, m_bufferId);
    }
}

void WebGpuBuffer::write(uint32_t offset, const void *data, uint32_t size)
{
    wgpu_write_buffer(m_deviceId, m_bufferId, offset, data, size);
}

void WebGpuBuffer::read(uint32_t offset, void *dst, uint32_t size)
{
    wgpu_read_buffer(m_deviceId, m_bufferId, offset, dst, size);
}

} // namespace xmrig
