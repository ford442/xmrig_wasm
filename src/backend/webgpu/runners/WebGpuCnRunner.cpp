/* XMRig
 * Copyright (c) 2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 */

#include "backend/webgpu/runners/WebGpuCnRunner.h"
#include "backend/webgpu/wrappers/WebGpuDevice.h"
#include "backend/webgpu/wrappers/WebGpuBuffer.h"
#include "backend/webgpu/wrappers/WebGpuKernel.h"
#include "base/crypto/Algorithm.h"
#include "base/net/stratum/Job.h"

#include <cstring>

namespace xmrig {

// TODO: load this from file or embedded string at build time
extern const char *cryptonight_wgsl;

WebGpuCnRunner::WebGpuCnRunner(size_t index, const WebGpuDevice &device)
    : WebGpuBaseRunner(index, device)
{
}

WebGpuCnRunner::~WebGpuCnRunner() = default;

size_t WebGpuCnRunner::bufferSize() const
{
    return m_memory;
}

void WebGpuCnRunner::build()
{
    WebGpuDevice dev(m_deviceId);

    m_scratchpads = std::make_unique<WebGpuBuffer>(dev, m_intensity * (m_memory >> 4) * 16, WebGpuBuffer::STORAGE);
    m_states      = std::make_unique<WebGpuBuffer>(dev, m_intensity * 25 * sizeof(uint64_t), WebGpuBuffer::STORAGE);

    // Create compute pipelines for each kernel entry point
    m_cn0 = std::make_unique<WebGpuKernel>(dev, cryptonight_wgsl, "cn0");
    m_cn1 = std::make_unique<WebGpuKernel>(dev, cryptonight_wgsl, "cn1");
    m_cn2 = std::make_unique<WebGpuKernel>(dev, cryptonight_wgsl, "cn2");
}

bool WebGpuCnRunner::run(uint32_t startNonce, uint32_t *rescount, uint32_t *resnonce)
{
    if (!m_cn0 || !m_cn1 || !m_cn2) {
        return false;
    }

    const uint32_t threads = static_cast<uint32_t>(m_intensity);

    // Bind arguments for cn0
    m_cn0->setArg(0, *m_inputBuffer);
    m_cn0->setArg(1, *m_scratchpads);
    m_cn0->setArg(2, *m_states);
    m_cn0->setArg(3, *m_outputBuffer);
    m_cn0->dispatch((threads + 63) / 64, 1, 1); // 64 threads per workgroup (8x8)
    m_cn0->enqueue();

    // Bind arguments for cn1
    m_cn1->setArg(0, *m_inputBuffer);
    m_cn1->setArg(1, *m_scratchpads);
    m_cn1->setArg(2, *m_states);
    m_cn1->setArg(3, *m_outputBuffer);
    m_cn1->dispatch((threads + m_worksize - 1) / m_worksize, 1, 1);
    m_cn1->enqueue();

    // Bind arguments for cn2
    m_cn2->setArg(0, *m_scratchpads);
    m_cn2->setArg(1, *m_states);
    m_cn2->setArg(2, *m_outputBuffer);
    m_cn2->dispatch((threads + 63) / 64, 1, 1);
    m_cn2->enqueue();

    // Read back results
    m_outputBuffer->read(0, rescount, sizeof(uint32_t) * 256);

    return true;
}

void WebGpuCnRunner::set(const Job &job, uint8_t *blob)
{
    // Write input blob to GPU buffer (already padded to 136 bytes by host)
    m_inputBuffer->write(0, blob, 136);
}

} // namespace xmrig
