/* XMRig
 * Copyright (c) 2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 */

#include "backend/webgpu/WebGpuLaunchData.h"
#include "core/Miner.h"

namespace xmrig {

WebGpuLaunchData::WebGpuLaunchData(const Miner *miner, const Algorithm &algorithm, const WebGpuThread &thread, const WebGpuDevice &device, int64_t affinity)
    : algorithm(algorithm)
    , affinity(affinity)
    , miner(miner)
    , device(device)
    , thread(thread)
{
    (void)this->device;
}

bool WebGpuLaunchData::isEqual(const WebGpuLaunchData &other) const
{
    return algorithm == other.algorithm &&
           affinity == other.affinity &&
           device.id() == other.device.id() &&
           thread == other.thread;
}

const char *WebGpuLaunchData::tag()
{
    return "webgpu";
}

} // namespace xmrig
