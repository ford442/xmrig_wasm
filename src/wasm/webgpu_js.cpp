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

/* ============================================================================
 * WebGPU JavaScript Bridge (Emscripten EM_JS)
 * ----------------------------------------------------------------------------
 * This file provides C++ → JavaScript wrappers for the browser WebGPU API.
 * It is only compiled for Emscripten/WASM builds (WITH_WEBGPU).
 *
 * Architecture:
 *   - All JS-side WebGPU objects (GPUDevice, GPUBuffer, GPUComputePipeline,
 *     GPUBindGroup, etc.) are stored in a JS-side WeakMap keyed by integer IDs.
 *   - C++ code passes integer IDs around and calls EM_JS functions to
 *     create, use, and destroy GPU resources.
 *   - This mirrors the OpenCL wrapper pattern (OclDevice, OclKernel, etc.)
 *     but targets the browser WebGPU API instead of OpenCL C APIs.
 *
 * TODO: Implement actual JS bodies. Current stubs return -1 to indicate
 *       "not yet implemented" so that the backend can gracefully fall back
 *       to CPU mining when WebGPU is unavailable.
 * ============================================================================ */

#include <emscripten.h>

extern "C" {

// Returns 1 if WebGPU is available in this browser, 0 otherwise.
EM_JS(int, wgpu_is_available, (), {
    return (typeof navigator !== 'undefined' && navigator.gpu) ? 1 : 0;
});

// Request adapter + device. Returns device ID (>0) on success, -1 on failure.
EM_JS(int, wgpu_create_device, (), {
    // TODO: implement async adapter/device request
    // navigator.gpu.requestAdapter() → adapter.requestDevice()
    // Store device in a global WeakMap, return an integer handle.
    return -1;
});

// Destroy a previously created device and free its resources.
EM_JS(void, wgpu_destroy_device, (int device_id), {
    // TODO
});

// Create a GPUBuffer. Returns buffer ID (>0) or -1 on failure.
EM_JS(int, wgpu_create_buffer, (int device_id, uint32_t size, int usage), {
    // TODO: usage flags map to GPUBufferUsage { STORAGE, COPY_SRC, COPY_DST, UNIFORM }
    return -1;
});

// Destroy a buffer.
EM_JS(void, wgpu_destroy_buffer, (int device_id, int buffer_id), {
    // TODO
});

// Write host data to a buffer (via staging / writeBuffer).
EM_JS(void, wgpu_write_buffer, (int device_id, int buffer_id, uint32_t offset, const void* data, uint32_t size), {
    // TODO: device.queue.writeBuffer(buf, offset, HEAPU8.subarray(data, data+size))
});

// Read buffer data back to host (mapAsync + getMappedRange).
EM_JS(void, wgpu_read_buffer, (int device_id, int buffer_id, uint32_t offset, void* dst, uint32_t size), {
    // TODO: async map, copy into HEAPU8, unmap
});

// Create a compute shader module from a WGSL source string.
EM_JS(int, wgpu_create_shader, (int device_id, const char* wgsl_source), {
    // TODO: device.createShaderModule({ code: UTF8ToString(wgsl_source) })
    return -1;
});

// Create a compute pipeline for a given entry point.
EM_JS(int, wgpu_create_pipeline, (int device_id, int shader_id, const char* entry_point), {
    // TODO: device.createComputePipeline({ layout: "auto", compute: { module, entryPoint } })
    return -1;
});

// Create a bind group linking buffers to a pipeline's layout.
EM_JS(int, wgpu_create_bind_group, (int device_id, int pipeline_id, const int* buffer_ids, const int* offsets, const int* sizes, int count), {
    // TODO: build GPUBindGroupEntries from buffer IDs and createBindGroup()
    return -1;
});

// Dispatch a compute pass: encode pipeline, bind group, dispatch, submit.
EM_JS(void, wgpu_dispatch, (
    int device_id,
    int pipeline_id,
    int bind_group_id,
    uint32_t workgroups_x,
    uint32_t workgroups_y,
    uint32_t workgroups_z
), {
    // TODO:
    // const encoder = device.createCommandEncoder();
    // const pass = encoder.beginComputePass();
    // pass.setPipeline(pipeline);
    // pass.setBindGroup(0, bindGroup);
    // pass.dispatchWorkgroups(x, y, z);
    // pass.end();
    // device.queue.submit([encoder.finish()]);
});

// Wait for all queued GPU operations to complete.
EM_JS(void, wgpu_flush, (int device_id), {
    // TODO: device.queue.onSubmittedWorkDone() or just let submit() handle it
});

} // extern "C"
