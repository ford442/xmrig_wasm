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

#ifndef XMRIG_WEBGPUQUEUE_H
#define XMRIG_WEBGPUQUEUE_H

#include "base/tools/Object.h"
#include <cstdint>

namespace xmrig {

class WebGpuDevice;

class WebGpuQueue
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(WebGpuQueue)

    explicit WebGpuQueue(const WebGpuDevice &device);
    ~WebGpuQueue();

    void flush();
    void waitIdle();

private:
    int m_deviceId = -1;
};

} // namespace xmrig

#endif // XMRIG_WEBGPUQUEUE_H
