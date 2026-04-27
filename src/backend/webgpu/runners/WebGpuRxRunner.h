/* XMRig
 * Copyright (c) 2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 */

#ifndef XMRIG_WEBGPURXRUNNER_H
#define XMRIG_WEBGPURXRUNNER_H

#include "backend/webgpu/runners/WebGpuBaseRunner.h"

namespace xmrig {

class WebGpuRxRunner : public WebGpuBaseRunner
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(WebGpuRxRunner)

    WebGpuRxRunner(size_t index, const WebGpuDevice &device);
    ~WebGpuRxRunner() override;

protected:
    size_t bufferSize() const override;
    bool run(uint32_t startNonce, uint32_t *rescount, uint32_t *resnonce) override;
    void set(const Job &job, uint8_t *blob) override;
    void build() override;
};

} // namespace xmrig

#endif // XMRIG_WEBGPURXRUNNER_H
