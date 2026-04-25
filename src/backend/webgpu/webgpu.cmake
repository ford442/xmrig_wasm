# WebGPU backend for XMRig WASM
# This backend translates OpenCL compute kernels to WGSL for browser-based GPU mining.

option(WITH_WEBGPU "Enable WebGPU backend (browser GPU mining via WGSL)" OFF)

if (WITH_WEBGPU)
    add_definitions(/DXMRIG_FEATURE_WEBGPU)
    message(STATUS "WebGPU backend enabled")
endif()

if (WITH_WEBGPU AND EMSCRIPTEN)
    message(STATUS "WebGPU backend: Emscripten JS bridge active")

    # List raw WGSL shader sources
    set(WGSL_CN_SOURCES
        src/backend/webgpu/shaders/wgsl/cn/cryptonight.wgsl
        src/backend/webgpu/shaders/wgsl/cn/wolf-aes.wgsl
        src/backend/webgpu/shaders/wgsl/cn/keccak.wgsl
    )

    set(WGSL_KAWPOW_SOURCES
        src/backend/webgpu/shaders/wgsl/kawpow/kawpow.wgsl
    )

    set(WGSL_ALL_SOURCES ${WGSL_CN_SOURCES} ${WGSL_KAWPOW_SOURCES})

    # TODO: build-time shader embedding (custom command to generate *_wgsl.h hex arrays)
    # For now shaders will be loaded as strings at runtime via EM_JS.

    set(HEADERS_BACKEND_WEBGPU
        src/backend/webgpu/WebGpuBackend.h
        src/backend/webgpu/WebGpuWorker.h
        src/backend/webgpu/WebGpuLaunchData.h
        src/backend/webgpu/WebGpuThread.h
        src/backend/webgpu/runners/WebGpuBaseRunner.h
        src/backend/webgpu/runners/WebGpuCnRunner.h
        src/backend/webgpu/runners/WebGpuRxRunner.h
        src/backend/webgpu/runners/WebGpuKawPowRunner.h
        src/backend/webgpu/wrappers/WebGpuDevice.h
        src/backend/webgpu/wrappers/WebGpuBuffer.h
        src/backend/webgpu/wrappers/WebGpuKernel.h
        src/backend/webgpu/wrappers/WebGpuQueue.h
    )

    set(SOURCES_BACKEND_WEBGPU
        src/backend/webgpu/WebGpuBackend.cpp
        src/backend/webgpu/WebGpuWorker.cpp
        src/backend/webgpu/WebGpuLaunchData.cpp
        src/backend/webgpu/WebGpuThread.cpp
        src/backend/webgpu/runners/WebGpuBaseRunner.cpp
        src/backend/webgpu/runners/WebGpuCnRunner.cpp
        src/backend/webgpu/runners/WebGpuRxRunner.cpp
        src/backend/webgpu/runners/WebGpuKawPowRunner.cpp
        src/backend/webgpu/wrappers/WebGpuDevice.cpp
        src/backend/webgpu/wrappers/WebGpuBuffer.cpp
        src/backend/webgpu/wrappers/WebGpuKernel.cpp
        src/backend/webgpu/wrappers/WebGpuQueue.cpp
    )

    # Add WASM-specific WebGPU JS bridge
    list(APPEND SOURCES_BACKEND_WEBGPU src/wasm/webgpu_js.cpp)

    include_directories(src/backend/webgpu)
endif()
