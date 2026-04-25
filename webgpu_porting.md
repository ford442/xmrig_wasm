# WebGPU WGSL Porting — Progress & Architecture

## Overview

This document tracks the progress of porting XMRig's GPU mining backends (OpenCL/CUDA) to **WebGPU WGSL Compute Shaders** for browser-based WASM mining.

**Approach:** OpenCL-to-WGSL Manual Translation (selected from approved plan).

---

## Completed Work

### 1. Agent Swarm Evaluation
Launched 4 parallel explore agents to analyze:
- **CUDA backend** (`src/backend/cuda/`) — found no in-repo kernels; backend is a plugin loader only
- **OpenCL backend** (`src/backend/opencl/`) — identified 3 algorithm families (CryptoNight, RandomX, KawPow) with raw `.cl` sources
- **WASM infrastructure** (`src/wasm/`, `CMakeLists.txt`) — confirmed Emscripten build disables all GPU backends
- **Mining algorithms** (`src/crypto/`) — assessed WGSL feasibility per algorithm

**Key finding:** Since no CUDA kernels exist in-repo, the OpenCL kernels are the authoritative translation source.

### 2. Porting Notes Added to Existing Files
Added `WEBGPU PORTING NOTES` comment blocks at the top of:
- `src/backend/opencl/cl/cn/cryptonight.cl`
- `src/backend/opencl/cl/cn/wolf-aes.cl`
- `src/backend/opencl/cl/cn/keccak.cl`
- `src/backend/opencl/cl/cn/fast_int_math_v2.cl`
- `src/backend/opencl/cl/cn/blake256.cl`
- `src/backend/opencl/cl/cn/groestl256.cl`
- `src/backend/cuda/wrappers/CudaLib.h`

### 3. New WebGPU Backend (`src/backend/webgpu/`)

#### CMake Integration
- `src/backend/webgpu/webgpu.cmake` — defines `WITH_WEBGPU` option, source lists, Emscripten JS bridge
- `src/backend/backend.cmake` — includes `webgpu.cmake`, adds WebGPU to `HEADERS_BACKEND` / `SOURCES_BACKEND`
- `CMakeLists.txt` — adds `WITH_WEBGPU` option, force-enables it for Emscripten
- `src/core/Miner.cpp` — instantiates `WebGpuBackend` when `XMRIG_FEATURE_WEBGPU` is defined
- `src/crypto/common/Nonce.h` + `Nonce.cpp` — added `WEBGPU` backend enum value

#### C++ ↔ JavaScript Bridge
- `src/wasm/webgpu_js.cpp` — `EM_JS` wrappers for all core WebGPU API calls:
  - `wgpu_is_available()`, `wgpu_create_device()`, `wgpu_destroy_device()`
  - `wgpu_create_buffer()`, `wgpu_write_buffer()`, `wgpu_read_buffer()`
  - `wgpu_create_shader()`, `wgpu_create_pipeline()`, `wgpu_create_bind_group()`
  - `wgpu_dispatch()`, `wgpu_flush()`

#### GPU Resource Wrappers (mirrors OpenCL wrapper pattern)
- `WebGpuDevice` — adapter/device enumeration, property queries
- `WebGpuBuffer` — storage buffer lifecycle (create, write, read, destroy)
- `WebGpuKernel` — compute pipeline + bind group helper, `setArg()` / `dispatch()`
- `WebGpuQueue` — command encoder / submit helper

#### Runner Architecture
- `WebGpuBaseRunner` — common buffer allocation, input/output buffer setup
- `WebGpuCnRunner` — CryptoNight pipeline (`cn0` → `cn1` → `cn2` dispatch)
- `WebGpuRxRunner` — RandomX placeholder (deferred until f64 support)
- `WebGpuKawPowRunner` — KawPow placeholder (Phase 2)

#### Backend Integration
- `WebGpuBackend` — implements `IBackend`, enables CN/CN_LITE for PoC
- `WebGpuWorker` — `GpuWorker` subclass, instantiates correct runner by algorithm
- `WebGpuLaunchData` — job launch configuration struct
- `WebGpuThread` — thread settings (intensity, worksize)

### 4. WGSL Shader Translation

#### `src/backend/webgpu/shaders/wgsl/cn/cryptonight.wgsl`
Complete CryptoNight Variant 0 (CN_0) compute shader with three entry points:

- **`cn0`** (`@workgroup_size(8, 8, 1)`)
  - Keccak-1600 init with nonce injection
  - AES-256 key expansion
  - AES scratchpad fill (2 MB per hash)

- **`cn1`** (`@workgroup_size(64, 1, 1)`)
  - Memory-hard main loop (524,288 iterations)
  - AES rounds + `mul_hi_u64` emulation
  - Scratchpad read/write with `MASK` indexing

- **`cn2`** (`@workgroup_size(8, 8, 1)`)
  - AES reverse walk over scratchpad
  - Final Keccak-1600
  - Output final state for host-side branch hashing

#### Key WGSL implementation details
- **Module-scope workgroup memory** for AES tables (`wg_aes0..3`) and state buffer (`wg_state`)
- **`mul_hi_u64`** emulated with 32-bit limb multiplication (WGSL has no built-in)
- **`rotate()`** emulated with shift/or
- **`bitselect64()`** emulated with bitwise ops
- All constants inlined (CN_0: `MEMORY=2MB`, `ITERATIONS=524288`, `MASK=0x1FFFF0`)

#### Supporting shaders
- `src/backend/webgpu/shaders/wgsl/cn/keccak.wgsl` — Keccak-f[1600] for standalone use
- `src/backend/webgpu/shaders/wgsl/cn/wolf-aes.wgsl` — AES round functions and tables

### 5. Build Verification
All new C++ files pass `g++ -fsyntax-only` checks (native Linux, not Emscripten).

---

## Remaining Work (PoC → Production)

### Phase 1 Completion (CryptoNight PoC)
1. **Implement JS bridge bodies** in `src/wasm/webgpu_js.cpp`
   - Wire up `navigator.gpu.requestAdapter()` → `requestDevice()`
   - Implement buffer creation, shader module compilation, pipeline creation
   - Implement `dispatchWorkgroups()` and read-back via `mapAsync()`

2. **Build-time shader embedding**
   - Replace `cryptonight_wgsl.cpp` stub with actual CMake custom command to convert `.wgsl` → C string
   - Or load WGSL at runtime via fetch() in the browser

3. **End-to-end browser test**
   - Update `wasm_test.html` to detect WebGPU and invoke `WebGpuCnRunner`
   - Verify hash output matches CPU reference for a single CN_0 hash

4. **Branch hash integration**
   - Add Blake-256, Groestl-256, JH-256, Skein-512 entry points to WGSL
   - Or implement branch selection + final hashing on CPU/WASM side

### Phase 2 (KawPow)
- Translate `kawpow.cl` + `kawpow_dag.cl` to WGSL
- Implement `WebGpuKawPowRunner`
- Handle DAG generation and per-epoch ProgPoW math sequences

### Phase 3 (RandomX — Deferred)
- Monitor browser `f64` support in WebGPU
- Translate `randomx_vm.cl` interpreter path
- Implement `WebGpuRxRunner` with light-mode dataset

### Phase 4 (Optimization)
- Profile workgroup sizes per vendor (Intel/Apple/AMD/NVIDIA)
- Cache compiled pipelines in `localStorage` / `IndexedDB`
- Optimize AES table initialization and shared memory usage

---

## File Map

```
src/backend/webgpu/
├── webgpu.cmake                          # CMake integration
├── WebGpuBackend.h/cpp                   # IBackend implementation
├── WebGpuWorker.h/cpp                    # GpuWorker subclass
├── WebGpuLaunchData.h/cpp                # Job launch config
├── WebGpuThread.h/cpp                    # Thread settings
├── runners/
│   ├── WebGpuBaseRunner.h/cpp            # Common runner logic
│   ├── WebGpuCnRunner.h/cpp              # CryptoNight pipeline
│   ├── WebGpuRxRunner.h/cpp              # RandomX placeholder
│   └── WebGpuKawPowRunner.h/cpp          # KawPow placeholder
├── wrappers/
│   ├── WebGpuDevice.h/cpp                # GPU device wrapper
│   ├── WebGpuBuffer.h/cpp                # Buffer wrapper
│   ├── WebGpuKernel.h/cpp                # Pipeline + bind group
│   └── WebGpuQueue.h/cpp                 # Command queue
└── shaders/
    ├── wgsl/cn/
    │   ├── cryptonight.wgsl              # Main CN_0 shader
    │   ├── keccak.wgsl                   # Keccak-f[1600]
    │   └── wolf-aes.wgsl                 # AES round functions
    └── generated/
        ├── cryptonight_wgsl.h            # Build-time embed stub
        └── cryptonight_wgsl.cpp          # Runtime string stub

src/wasm/
└── webgpu_js.cpp                         # EM_JS WebGPU API bridge
```

---

## Feasibility Recap

| Algorithm | Status | Notes |
|-----------|--------|-------|
| **CryptoNight** | 🟡 In Progress | WGSL shader complete; JS bridge stubs need implementation |
| **KawPow** | 🔴 Not Started | Placeholder runners only |
| **RandomX** | 🔴 Blocked | `f64` + dynamic rounding not available in browsers |
| **GhostRider** | 🔴 Not Feasible | 15 hashes + 6 CN variants = too large for single shader |
| **Argon2** | 🟢 No GPU kernel | Better left on CPU via WASM SIMD |
