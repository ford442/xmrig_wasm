/* XMRig
 * Copyright (c) 2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 */

#include "backend/webgpu/WebGpuThread.h"

namespace xmrig {

bool WebGpuThread::isEqual(const WebGpuThread &other) const
{
    return m_index == other.m_index &&
           m_intensity == other.m_intensity &&
           m_worksize == other.m_worksize &&
           m_threads == other.m_threads;
}

} // namespace xmrig
