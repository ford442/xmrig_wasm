/* XMRig
 * Copyright (c) 2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 */

#ifndef XMRIG_WEBGPUBACKEND_H
#define XMRIG_WEBGPUBACKEND_H

#include "backend/common/interfaces/IBackend.h"
#include "base/tools/Object.h"
#include "base/tools/String.h"
#include <vector>
#include <memory>

namespace xmrig {

class Controller;

class WebGpuBackend : public IBackend
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(WebGpuBackend)

    WebGpuBackend(Controller *controller);
    ~WebGpuBackend() override;

    bool isEnabled() const override;
    bool isEnabled(const Algorithm &algorithm) const override;
    const Hashrate *hashrate() const override { return nullptr; }
    const String &profileName() const override { return m_deviceName; }
    const String &type() const override;
    void execCommand(char) override {}
    void prepare(const Job&) override {}
    void printHashrate(bool details) override {}
    void printHealth() override {}
    void setJob(const Job &job) override;
    void start(IWorker *worker, bool ready) override {}
    void stop() override {}
    bool tick(uint64_t ticks) override { return true; }

#   ifdef XMRIG_FEATURE_API
    rapidjson::Value toJSON(rapidjson::Document &doc) const override;
    void handleRequest(IApiRequest&) override {}
#   endif

#   ifdef XMRIG_FEATURE_BENCHMARK
    inline Benchmark *benchmark() const override        { return nullptr; }
    inline void printBenchProgress() const override     {}
#   endif

private:
    bool m_enabled = false;
    String m_deviceName;
};

} // namespace xmrig

#endif // XMRIG_WEBGPUBACKEND_H
