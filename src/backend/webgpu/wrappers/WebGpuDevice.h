/* XMRig
 * Copyright (c) 2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef XMRIG_WEBGPUDEVICE_H
#define XMRIG_WEBGPUDEVICE_H

#include "base/tools/Object.h"
#include <cstdint>
#include <string>
#include <vector>

namespace xmrig {

class WebGpuDevice
{
public:
    WebGpuDevice() = default;
    explicit WebGpuDevice(int deviceId);
    ~WebGpuDevice();

    inline bool isValid() const     { return m_deviceId > 0; }
    inline int id() const           { return m_deviceId; }

    std::string name() const;
    uint32_t maxWorkGroupSize() const;
    uint64_t maxBufferSize() const;
    uint64_t maxStorageBufferBindingSize() const;

    static std::vector<WebGpuDevice> enumerate();
    static bool isAvailable();

private:
    int m_deviceId = -1;
};

} // namespace xmrig

#endif // XMRIG_WEBGPUDEVICE_H
