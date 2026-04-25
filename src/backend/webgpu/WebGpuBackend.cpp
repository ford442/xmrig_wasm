/* XMRig
 * Copyright (c) 2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 */

#include "backend/webgpu/WebGpuBackend.h"
#include "backend/webgpu/wrappers/WebGpuDevice.h"
#include "base/crypto/Algorithm.h"
#include "core/Controller.h"
#include "3rdparty/rapidjson/document.h"

namespace xmrig {

WebGpuBackend::WebGpuBackend(Controller *controller)
    : m_enabled(WebGpuDevice::isAvailable())
{
    if (m_enabled) {
        auto devices = WebGpuDevice::enumerate();
        if (!devices.empty()) {
            m_deviceName = devices.front().name().c_str();
        }
    }
}

WebGpuBackend::~WebGpuBackend() = default;

bool WebGpuBackend::isEnabled() const
{
    return m_enabled;
}

bool WebGpuBackend::isEnabled(const Algorithm &algorithm) const
{
    // PoC only supports CryptoNight family
    return m_enabled && (algorithm.family() == Algorithm::CN || algorithm.family() == Algorithm::CN_LITE);
}

const String &WebGpuBackend::type() const
{
    static const String kType = "webgpu";
    return kType;
}

void WebGpuBackend::setJob(const Job &job)
{
    // TODO: forward job to workers
}

#   ifdef XMRIG_FEATURE_API
rapidjson::Value WebGpuBackend::toJSON(rapidjson::Document &doc) const
{
    return rapidjson::Value();
}
#   endif

} // namespace xmrig
