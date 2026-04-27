/* XMRig
 * Copyright (c) 2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 */

#ifndef XMRIG_WEBGPUWORKER_H
#define XMRIG_WEBGPUWORKER_H

#include "backend/common/GpuWorker.h"
#include "backend/common/WorkerJob.h"
#include "backend/webgpu/WebGpuLaunchData.h"
#include "base/tools/Object.h"
#include "net/JobResult.h"

namespace xmrig {

class WebGpuBaseRunner;
class Job;

class WebGpuWorker : public GpuWorker
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(WebGpuWorker)

    WebGpuWorker(size_t id, const WebGpuLaunchData &data);
    ~WebGpuWorker() override;

    void jobEarlyNotification(const Job &job) override;

protected:
    bool selfTest() override;
    size_t intensity() const override;
    void start() override;

private:
    bool consumeJob();

    const Algorithm m_algorithm;
    const Miner *m_miner;
    WebGpuBaseRunner *m_runner = nullptr;
    WorkerJob<1> m_job;
};

} // namespace xmrig

#endif // XMRIG_WEBGPUWORKER_H
