/* XMRig
 * Copyright (c) 2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 */

#ifndef XMRIG_WEBGPUCNRUNNER_H
#define XMRIG_WEBGPUCNRUNNER_H

#include "backend/webgpu/runners/WebGpuBaseRunner.h"
#include <memory>
#include <vector>

namespace xmrig {

class WebGpuKernel;
class WebGpuBuffer;
class Job;

class WebGpuCnRunner : public WebGpuBaseRunner
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(WebGpuCnRunner)

    WebGpuCnRunner(size_t index, const WebGpuDevice &device);
    ~WebGpuCnRunner() override;

protected:
    size_t bufferSize() const override;
    bool run(uint32_t startNonce, uint32_t *rescount, uint32_t *resnonce) override;
    void set(const Job &job, uint8_t *blob) override;
    void build() override;

private:
    enum Branches : size_t {
        BRANCH_BLAKE_256,
        BRANCH_GROESTL_256,
        BRANCH_JH_256,
        BRANCH_SKEIN_512,
        BRANCH_MAX
    };

    std::unique_ptr<WebGpuBuffer> m_scratchpads;
    std::unique_ptr<WebGpuBuffer> m_states;
    std::unique_ptr<WebGpuBuffer> m_branchBuffers[BRANCH_MAX];

    std::unique_ptr<WebGpuKernel> m_cn0;
    std::unique_ptr<WebGpuKernel> m_cn1;
    std::unique_ptr<WebGpuKernel> m_cn2;
    std::vector<std::unique_ptr<WebGpuKernel>> m_branchKernels;

    uint64_t m_height = 0;
    uint32_t m_worksize = 64;
    uint32_t m_memory = 2097152; // 2 MB for CN_0
};

} // namespace xmrig

#endif // XMRIG_WEBGPUCNRUNNER_H
