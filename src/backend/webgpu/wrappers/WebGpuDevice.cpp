/* XMRig
 * Copyright (c) 2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 */

#include "backend/webgpu/wrappers/WebGpuDevice.h"

extern "C" {
    int wgpu_is_available();
    int wgpu_create_device();
    void wgpu_destroy_device(int device_id);
}

namespace xmrig {

WebGpuDevice::WebGpuDevice(int deviceId) : m_deviceId(deviceId) {}

WebGpuDevice::~WebGpuDevice()
{
    if (m_deviceId > 0) {
        wgpu_destroy_device(m_deviceId);
    }
}

std::string WebGpuDevice::name() const
{
    return "WebGPU Device";
}

uint32_t WebGpuDevice::maxWorkGroupSize() const
{
    // WebGPU minimum guaranteed limit
    return 256;
}

uint64_t WebGpuDevice::maxBufferSize() const
{
    // TODO: query from adapter.limits
    return 268435456; // 256 MB default
}

uint64_t WebGpuDevice::maxStorageBufferBindingSize() const
{
    return 134217728; // 128 MB default
}

std::vector<WebGpuDevice> WebGpuDevice::enumerate()
{
    std::vector<WebGpuDevice> devices;
    if (!isAvailable()) {
        return devices;
    }

    int id = wgpu_create_device();
    if (id > 0) {
        devices.emplace_back(id);
    }
    return devices;
}

bool WebGpuDevice::isAvailable()
{
    return wgpu_is_available() != 0;
}

} // namespace xmrig
