/* XMRig
 * Copyright (c) 2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 */

#ifndef XMRIG_WEBGPUTHREAD_H
#define XMRIG_WEBGPUTHREAD_H

#include <cstdint>
#include <vector>

namespace xmrig {

class WebGpuThread
{
public:
    WebGpuThread() = delete;
    WebGpuThread(uint32_t index, uint32_t intensity, uint32_t worksize, uint32_t threads) :
        m_threads(threads, -1),
        m_index(index),
        m_worksize(worksize)
    {
        setIntensity(intensity);
    }

    inline bool isValid() const                             { return m_intensity > 0; }
    inline const std::vector<int64_t> &threads() const      { return m_threads; }
    inline uint32_t index() const                           { return m_index; }
    inline uint32_t intensity() const                       { return m_intensity; }
    inline uint32_t worksize() const                        { return m_worksize; }

    inline bool operator!=(const WebGpuThread &other) const { return !isEqual(other); }
    inline bool operator==(const WebGpuThread &other) const { return isEqual(other); }

    bool isEqual(const WebGpuThread &other) const;

private:
    inline void setIntensity(uint32_t intensity) { m_intensity = intensity / m_worksize * m_worksize; }

    std::vector<int64_t> m_threads;
    uint32_t m_index = 0;
    uint32_t m_intensity = 0;
    uint32_t m_worksize = 0;
};

} // namespace xmrig

#endif // XMRIG_WEBGPUTHREAD_H
