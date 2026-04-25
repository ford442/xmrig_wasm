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

#ifndef XMRIG_WEBGPUKERNEL_H
#define XMRIG_WEBGPUKERNEL_H

#include "base/tools/Object.h"
#include "base/tools/String.h"
#include <cstdint>
#include <vector>

namespace xmrig {

class WebGpuDevice;
class WebGpuBuffer;

class WebGpuKernel
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(WebGpuKernel)

    WebGpuKernel(const WebGpuDevice &device, const char *wgslSource, const char *entryPoint);
    ~WebGpuKernel();

    inline bool isValid() const         { return m_pipelineId > 0; }
    inline int pipelineId() const       { return m_pipelineId; }
    inline const String &name() const   { return m_name; }

    // Set buffer arguments before dispatch. Call in binding-index order.
    void setArg(uint32_t index, const WebGpuBuffer &buffer, uint32_t offset = 0, uint32_t size = 0);

    // Dispatch compute workgroups.
    void dispatch(uint32_t workgroupsX, uint32_t workgroupsY = 1, uint32_t workgroupsZ = 1);

    // Submit the encoded command buffer to the GPU queue.
    void enqueue();

    // Create the bind group from accumulated setArg calls.
    void buildBindGroup();

private:
    int m_deviceId = -1;
    int m_pipelineId = -1;
    int m_bindGroupId = -1;
    String m_name;

    struct Arg
    {
        int bufferId = -1;
        uint32_t offset = 0;
        uint32_t size = 0;
    };
    std::vector<Arg> m_args;
    bool m_argsDirty = false;
};

} // namespace xmrig

#endif // XMRIG_WEBGPUKERNEL_H
