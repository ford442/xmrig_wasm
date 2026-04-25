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

#ifndef XMRIG_WEBGPUBUFFER_H
#define XMRIG_WEBGPUBUFFER_H

#include "base/tools/Object.h"
#include <cstdint>

namespace xmrig {

class WebGpuDevice;

class WebGpuBuffer
{
public:
    enum Usage : uint32_t
    {
        STORAGE     = 1 << 0,   // read_write storage buffer
        COPY_SRC    = 1 << 1,   // can be source of copy
        COPY_DST    = 1 << 2,   // can be destination of copy
        UNIFORM     = 1 << 3,   // uniform buffer
    };

    XMRIG_DISABLE_COPY_MOVE_DEFAULT(WebGpuBuffer)

    WebGpuBuffer(const WebGpuDevice &device, uint32_t size, uint32_t usage);
    ~WebGpuBuffer();

    inline bool isValid() const     { return m_bufferId > 0; }
    inline int id() const           { return m_bufferId; }
    inline uint32_t size() const    { return m_size; }

    void write(uint32_t offset, const void *data, uint32_t size);
    void read(uint32_t offset, void *dst, uint32_t size);

private:
    int m_deviceId = -1;
    int m_bufferId = -1;
    uint32_t m_size = 0;
};

} // namespace xmrig

#endif // XMRIG_WEBGPUBUFFER_H
