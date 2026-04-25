/* XMRig
 * Copyright (c) 2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 */

#ifndef XMRIG_WEBGPUKAWPOWRUNNER_H
#define XMRIG_WEBGPUKAWPOWRUNNER_H

#include "backend/webgpu/runners/WebGpuBaseRunner.h"

namespace xmrig {

class WebGpuKawPowRunner : public WebGpuBaseRunner
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(WebGpuKawPowRunner)

    WebGpuKawPowRunner(size_t index, const WebGpuDevice &device);
    ~WebGpuKawPowRunner() override;

protected:
    size_t bufferSize() const override;
    bool run(uint32_t startNonce, uint32_t *rescount, uint32_t *resnonce) override;
    void set(const Job &job, uint8_t *blob) override;
    void build() override;
};

} // namespace xmrig

#endif // XMRIG_WEBGPUKAWPOWRUNNER_H
