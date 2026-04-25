/* XMRig
 * Copyright (c) 2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 */

#ifndef XMRIG_WEBGPULAUNCHDATA_H
#define XMRIG_WEBGPULAUNCHDATA_H

#include "backend/webgpu/WebGpuThread.h"
#include "backend/webgpu/wrappers/WebGpuDevice.h"
#include "base/crypto/Algorithm.h"
#include "crypto/common/Nonce.h"

namespace xmrig {

class Miner;

class WebGpuLaunchData
{
public:
    WebGpuLaunchData(const Miner *miner, const Algorithm &algorithm, const WebGpuThread &thread, const WebGpuDevice &device, int64_t affinity);

    bool isEqual(const WebGpuLaunchData &other) const;

    inline constexpr static Nonce::Backend backend() { return Nonce::WEBGPU; }
    inline bool operator!=(const WebGpuLaunchData &other) const { return !isEqual(other); }
    inline bool operator==(const WebGpuLaunchData &other) const { return isEqual(other); }

    static const char *tag();

    const Algorithm algorithm;
    const int64_t affinity;
    const Miner *miner;
    const WebGpuDevice &device;
    const WebGpuThread thread;
};

} // namespace xmrig

#endif // XMRIG_WEBGPULAUNCHDATA_H
