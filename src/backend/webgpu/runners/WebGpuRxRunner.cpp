/* XMRig
 * Copyright (c) 2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 */

#include "backend/webgpu/runners/WebGpuRxRunner.h"
#include "backend/webgpu/wrappers/WebGpuDevice.h"

namespace xmrig {

WebGpuRxRunner::WebGpuRxRunner(size_t index, const WebGpuDevice &device)
    : WebGpuBaseRunner(index, device)
{
}

WebGpuRxRunner::~WebGpuRxRunner() = default;

size_t WebGpuRxRunner::bufferSize() const
{
    return 0; // Not yet implemented
}

bool WebGpuRxRunner::run(uint32_t startNonce, uint32_t *rescount, uint32_t *resnonce)
{
    return false; // RandomX deferred until f64 support
}

void WebGpuRxRunner::set(const Job &job, uint8_t *blob)
{
}

void WebGpuRxRunner::build()
{
}

} // namespace xmrig
