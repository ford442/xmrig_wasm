/* XMRig
 * Copyright (c) 2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 */

#include "backend/webgpu/wrappers/WebGpuQueue.h"
#include "backend/webgpu/wrappers/WebGpuDevice.h"

extern "C" {
    void wgpu_flush(int device_id);
}

namespace xmrig {

WebGpuQueue::WebGpuQueue(const WebGpuDevice &device) : m_deviceId(device.id()) {}
WebGpuQueue::~WebGpuQueue() = default;

void WebGpuQueue::flush()
{
    wgpu_flush(m_deviceId);
}

void WebGpuQueue::waitIdle()
{
    // TODO: map a dummy buffer and await to guarantee idle
    wgpu_flush(m_deviceId);
}

} // namespace xmrig
