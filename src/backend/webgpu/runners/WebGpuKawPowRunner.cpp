/* XMRig
 * Copyright (c) 2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 */

#include "backend/webgpu/runners/WebGpuKawPowRunner.h"
#include "backend/webgpu/wrappers/WebGpuDevice.h"

namespace xmrig {

WebGpuKawPowRunner::WebGpuKawPowRunner(size_t index, const WebGpuDevice &device)
    : WebGpuBaseRunner(index, device)
{
}

WebGpuKawPowRunner::~WebGpuKawPowRunner() = default;

size_t WebGpuKawPowRunner::bufferSize() const
{
    return 0; // Not yet implemented
}

bool WebGpuKawPowRunner::run(uint32_t startNonce, uint32_t *rescount, uint32_t *resnonce)
{
    return false; // KawPow Phase 2
}

void WebGpuKawPowRunner::set(const Job &job, uint8_t *blob)
{
}

void WebGpuKawPowRunner::build()
{
}

} // namespace xmrig
