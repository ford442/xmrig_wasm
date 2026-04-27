/* XMRig
 * Copyright (c) 2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 */

#ifndef XMRIG_WEBGPUBASERUNNER_H
#define XMRIG_WEBGPUBASERUNNER_H

#include "base/tools/Object.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace xmrig {

class Job;
class WebGpuDevice;
class WebGpuBuffer;
class WebGpuKernel;

class WebGpuBaseRunner
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(WebGpuBaseRunner)

    WebGpuBaseRunner(size_t index, const WebGpuDevice &device);
    virtual ~WebGpuBaseRunner();

    bool init();
    inline size_t intensity() const { return m_intensity; }
    virtual bool run(uint32_t startNonce, uint32_t *rescount, uint32_t *resnonce) = 0;
    virtual void set(const Job &job, uint8_t *blob) = 0;
    virtual void jobEarlyNotification(const Job&) {}

protected:
    size_t m_index = 0;
    int m_deviceId = -1;
    uint32_t m_intensity = 0;

    std::unique_ptr<WebGpuBuffer> m_inputBuffer;
    std::unique_ptr<WebGpuBuffer> m_outputBuffer;

    virtual size_t bufferSize() const = 0;
    virtual void build() = 0;
};

} // namespace xmrig

#endif // XMRIG_WEBGPUBASERUNNER_H
