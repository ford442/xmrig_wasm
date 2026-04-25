/* XMRig
 * Copyright (c) 2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 */

#include "backend/webgpu/WebGpuWorker.h"
#include "backend/webgpu/WebGpuLaunchData.h"
#include "backend/webgpu/runners/WebGpuCnRunner.h"
#include "backend/webgpu/runners/WebGpuBaseRunner.h"
#include "base/net/stratum/Job.h"
#include "core/Miner.h"

namespace xmrig {

WebGpuWorker::WebGpuWorker(size_t id, const WebGpuLaunchData &data)
    : GpuWorker(id, data.affinity, -1, data.thread.index())
    , m_algorithm(data.algorithm)
    , m_miner(data.miner)
{
    // TODO: instantiate correct runner based on algorithm family
    if (data.algorithm.family() == Algorithm::CN || data.algorithm.family() == Algorithm::CN_LITE) {
        m_runner = static_cast<WebGpuBaseRunner*>(new WebGpuCnRunner(id, data.device));
    }

    if (m_runner && !m_runner->init()) {
        delete m_runner;
        m_runner = nullptr;
    }
}

WebGpuWorker::~WebGpuWorker()
{
    delete m_runner;
}

void WebGpuWorker::jobEarlyNotification(const Job &job)
{
    if (m_runner) {
        m_runner->jobEarlyNotification(job);
    }
}

bool WebGpuWorker::selfTest()
{
    return m_runner != nullptr;
}

size_t WebGpuWorker::intensity() const
{
    return m_runner ? m_runner->intensity() : 0;
}

void WebGpuWorker::start()
{
    // TODO: implement work loop
}

} // namespace xmrig
