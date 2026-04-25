/* XMRig
 * Copyright (c) 2024 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 */

#include "backend/webgpu/wrappers/WebGpuKernel.h"
#include "backend/webgpu/wrappers/WebGpuDevice.h"
#include "backend/webgpu/wrappers/WebGpuBuffer.h"

extern "C" {
    int wgpu_create_shader(int device_id, const char* wgsl_source);
    int wgpu_create_pipeline(int device_id, int shader_id, const char* entry_point);
    int wgpu_create_bind_group(int device_id, int pipeline_id, const int* buffer_ids, const int* offsets, const int* sizes, int count);
    void wgpu_dispatch(int device_id, int pipeline_id, int bind_group_id, uint32_t workgroups_x, uint32_t workgroups_y, uint32_t workgroups_z);
}

namespace xmrig {

WebGpuKernel::WebGpuKernel(const WebGpuDevice &device, const char *wgslSource, const char *entryPoint)
    : m_deviceId(device.id())
    , m_name(entryPoint)
{
    int shaderId = wgpu_create_shader(m_deviceId, wgslSource);
    if (shaderId > 0) {
        m_pipelineId = wgpu_create_pipeline(m_deviceId, shaderId, entryPoint);
    }
}

WebGpuKernel::~WebGpuKernel()
{
    // TODO: destroy pipeline and shader module via JS bridge
}

void WebGpuKernel::setArg(uint32_t index, const WebGpuBuffer &buffer, uint32_t offset, uint32_t size)
{
    if (index >= m_args.size()) {
        m_args.resize(index + 1);
    }
    m_args[index] = { buffer.id(), offset, size ? size : buffer.size() };
    m_argsDirty = true;
}

void WebGpuKernel::buildBindGroup()
{
    if (!m_argsDirty || m_pipelineId <= 0) {
        return;
    }

    std::vector<int> bufferIds;
    std::vector<int> offsets;
    std::vector<int> sizes;
    for (const auto &arg : m_args) {
        bufferIds.push_back(arg.bufferId);
        offsets.push_back(static_cast<int>(arg.offset));
        sizes.push_back(static_cast<int>(arg.size));
    }

    m_bindGroupId = wgpu_create_bind_group(
        m_deviceId,
        m_pipelineId,
        bufferIds.data(),
        offsets.data(),
        sizes.data(),
        static_cast<int>(m_args.size())
    );
    m_argsDirty = false;
}

void WebGpuKernel::dispatch(uint32_t workgroupsX, uint32_t workgroupsY, uint32_t workgroupsZ)
{
    buildBindGroup();
    if (m_pipelineId > 0 && m_bindGroupId > 0) {
        wgpu_dispatch(m_deviceId, m_pipelineId, m_bindGroupId, workgroupsX, workgroupsY, workgroupsZ);
    }
}

void WebGpuKernel::enqueue()
{
    // In WebGPU, dispatch already submits via command encoder.
    // This method exists for API parity with OpenCL backend.
}

} // namespace xmrig
