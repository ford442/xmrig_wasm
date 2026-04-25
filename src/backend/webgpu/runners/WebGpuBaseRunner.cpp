/* XMRig
 * Copyright (c) 2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 */

#include "backend/webgpu/runners/WebGpuBaseRunner.h"
#include "backend/webgpu/wrappers/WebGpuDevice.h"
#include "backend/webgpu/wrappers/WebGpuBuffer.h"
#include "base/net/stratum/Job.h"

namespace xmrig {

WebGpuBaseRunner::WebGpuBaseRunner(size_t index, const WebGpuDevice &device)
    : m_index(index)
    , m_deviceId(device.id())
{
}

WebGpuBaseRunner::~WebGpuBaseRunner() = default;

bool WebGpuBaseRunner::init()
{
    if (m_deviceId <= 0) {
        return false;
    }

    const size_t size = bufferSize();
    if (size == 0) {
        return false;
    }

    m_inputBuffer  = std::make_unique<WebGpuBuffer>(WebGpuDevice(m_deviceId), 136, WebGpuBuffer::STORAGE | WebGpuBuffer::COPY_DST);
    m_outputBuffer = std::make_unique<WebGpuBuffer>(WebGpuDevice(m_deviceId), 256 * sizeof(uint32_t), WebGpuBuffer::STORAGE | WebGpuBuffer::COPY_SRC);

    build();
    return true;
}

} // namespace xmrig
