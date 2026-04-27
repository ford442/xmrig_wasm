# AGENTS.md — xmrig_wasm

This file is written for AI coding agents. It assumes you know nothing about the project.

---

## Project Overview

**xmrig_wasm** is a WebAssembly port of [XMRig](https://github.com/xmrig/xmrig), a high-performance, open-source, cross-platform CPU/GPU miner for the RandomX, KawPow, CryptoNight, GhostRider, and Argon2 algorithm families.

- **Upstream:** `xmrig/xmrig`  
- **This fork:** `ford442/xmrig_wasm`  
- **Version:** 6.26.0 (C++ core), 3.0.0 (WASM build wrapper in `package.json`)  
- **License:** GPL-3.0  
- **Primary language:** C++11  

The main goal of this fork is to compile XMRig to WebAssembly using the Emscripten toolchain so it can run inside a web browser. A new **WebGPU backend** (`src/backend/webgpu/`) is also being developed to offload mining compute to the browser's GPU via WGSL compute shaders.

> **Important:** This is a technical port and proof-of-concept. Browser-based mining has serious ethical and legal implications. Performance in WASM is orders of magnitude slower than native builds.

---

## Technology Stack

| Layer | Technology |
|-------|------------|
| Core language | C++11, C99 |
| Build system | CMake 3.10+ |
| WASM toolchain | Emscripten SDK (`emsdk`) |
| Build orchestration | Node.js 14+, npm |
| Browser testing | Playwright 1.59.1, `ws` 8.20.0 |
| Embedded 3rdparty | rapidjson, fmt, llhttp, libethash, argon2, getopt, hwloc headers, OpenCL headers |

### Key CMake options (from `CMakeLists.txt`)

Most feature flags are `ON` by default for native builds, but `CMakeLists.txt` **force-disables** the following when `EMSCRIPTEN` is detected:

- `WITH_HWLOC`, `WITH_TLS`, `WITH_MSR`, `WITH_DMI`
- `WITH_OPENCL`, `WITH_CUDA`, `WITH_NVML`, `WITH_ADL`
- `WITH_GHOSTRIDER`, `WITH_SSE4_1`, `WITH_AVX2`, `WITH_VAES`, `WITH_ASM`, `WITH_SECURE_JIT`, `WITH_HTTP`

It also **force-enables** `WITH_WEBGPU` for Emscripten builds.

---

## Directory Structure

```
├── CMakeLists.txt              # Main CMake configuration
├── package.json                # npm scripts and dependencies
├── cmake/                      # CMake modules (flags, CPU, OS, OpenSSL, asm, etc.)
├── scripts/
│   └── wasm_build.js           # Node helper: configures, builds, cleans, serves
├── wasm_test.html              # Browser test harness (single-page HTML/JS)
├── deploy.py                   # Paramiko/SFTP deployment script (hardcoded creds)
├── src/
│   ├── App.cpp / App.h         # Main application controller
│   ├── App_unix.cpp            # Native daemon/background mode
│   ├── App_wasm.cpp            # WASM stub (no fork/setsid)
│   ├── xmrig.cpp               # Entry point (main)
│   ├── Summary.cpp / Summary.h # Startup info printer
│   ├── version.h               # Version macros (6.26.0)
│   ├── config.json             # Default embedded miner config
│   │
│   ├── base/                   # Core infrastructure
│   │   ├── api/                # HTTP API server (disabled for WASM)
│   │   ├── crypto/             # Algorithm, Coin, keccak, sha3
│   │   ├── io/                 # Console, logging, JSON, Async, Signals, Watcher
│   │   ├── kernel/             # Base, Entry, Platform, Process, Config transforms
│   │   ├── net/                # DNS, HTTP, Stratum/TLS client, pool strategies
│   │   └── tools/              # String, Buffer, Timer, Chrono, cryptonote utils
│   │
│   ├── backend/                # Mining backends
│   │   ├── common/             # Hashrate, Tags, Thread, Workers, benchmarks
│   │   ├── cpu/                # CPU backend (x86/ARM/RISC-V asm & intrinsics)
│   │   ├── opencl/             # OpenCL backend (disabled for WASM)
│   │   ├── cuda/               # CUDA backend (disabled for WASM)
│   │   └── webgpu/             # NEW WebGPU backend (WGSL shaders + C++ wrappers)
│   │
│   ├── crypto/                 # Algorithm implementations
│   │   ├── cn/                 # CryptoNight family (C + x86/ARM asm)
│   │   ├── randomx/            # RandomX VM, JIT compilers, interpreter
│   │   ├── rx/                 # RandomX integration (RxCache, RxDataset, RxQueue, RxConfig)
│   │   ├── argon2/             # Argon2 family
│   │   ├── kawpow/             # KawPow (ethash + ProgPoW)
│   │   ├── ghostrider/         # GhostRider (disabled for WASM)
│   │   └── common/             # VirtualMemory, MemoryPool, Nonce
│   │
│   ├── hw/                     # Hardware introspection
│   │   ├── api/                # CPU info wrappers
│   │   ├── dmi/                # DMI/SMBIOS reader (disabled for WASM)
│   │   └── msr/                # MSR mod (disabled for WASM)
│   │
│   ├── net/                    # High-level networking
│   │   └── strategies/         # DonateStrategy, FailoverStrategy
│   │
│   ├── wasm/                   # WASM-specific shims
│   │   ├── uv.h                # libuv-compatible header for WASM
│   │   ├── uv_wasm.cpp         # libuv shim implementation (timers, async, loop stubs)
│   │   └── webgpu_js.cpp       # EM_JS bridge stubs for browser WebGPU API
│   │
│   └── 3rdparty/               # Embedded dependencies
│       ├── rapidjson/          # JSON parsing/generation
│       ├── fmt/                # String formatting
│       ├── llhttp/             # HTTP parser
│       ├── libethash/          # Ethash DAG helpers
│       ├── argon2/             # Argon2 reference implementation
│       ├── getopt/             # CLI option parsing (Windows)
│       ├── hwloc/              # Hardware locality headers
│       └── CL/                 # OpenCL C headers
```

---

## Build Commands

### Prerequisites

**Native build:**
```bash
sudo apt update
sudo apt install libhwloc-dev libuv1-dev cmake build-essential
```

**WASM build:**
```bash
# Install Emscripten SDK
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

### Native Build (Linux)

```bash
mkdir -p build && cd build
cmake ..
cmake --build . -j$(nproc)
# Output: ./xmrig
```

### WASM Build

The project provides npm scripts that wrap `emcmake` and `emmake`:

```bash
# Configure
npm run configure

# Build (defaults to Release)
npm run build

# Debug or Release explicitly
npm run build:debug
npm run build:release

# Clean build directory
npm run clean

# Build + start local test server
npm run dev

# Just start the server (serves build_wasm/ on port 8000)
npm run serve
```

**Manual CMake (equivalent to `npm run configure && npm run build`):**
```bash
mkdir -p build_wasm && cd build_wasm
emcmake cmake \
  -DXMRIG_FEATURE_RANDOMX=ON \
  -DXMRIG_FEATURE_CN=ON \
  -DCMAKE_BUILD_TYPE=Release \
  ..
emmake make -j$(nproc)
```

**Build artifacts:**
- `build_wasm/xmrig-notls.js` — Emscripten JavaScript runtime wrapper
- `build_wasm/xmrig-notls.wasm` — WebAssembly binary

### Emscripten Linker Flags (from `cmake/flags.cmake`)

When the compiler is Emscripten, the following flags are set:

```
-s WASM=1
-s ALLOW_MEMORY_GROWTH=1
-s INITIAL_MEMORY=536870912
-s MAXIMUM_MEMORY=4294967296
-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','callMain','FS']
-s EXPORTED_FUNCTIONS=['_main','_malloc','_free']
-s USE_PTHREADS=1
-s PTHREAD_POOL_SIZE=16
-s PTHREAD_POOL_SIZE_STRICT=0
```

Compile flags: `-pthread -msimd128 -msse -msse2`

---

## Testing Instructions

There is **no unit test framework** in this repository (no gtest, Catch2, etc.). Testing is manual and build-centric.

### 1. Compilation as correctness check
A successful build with zero errors is the primary validation step.

### 2. Browser test harness (`wasm_test.html`)

```bash
npm run dev
# Opens http://localhost:8000/wasm_test.html automatically
```

The harness lets you:
- Select an algorithm (RandomX, CryptoNight, CryptoNight Lite)
- Set test duration, hash count, and thread count
- Start/stop a mining simulation
- View hashes completed, hash rate, elapsed time, and log output

**Requirements for the server:**
- Must serve over HTTP (not `file://`)
- Must send `Cross-Origin-Opener-Policy: same-origin` and `Cross-Origin-Embedder-Policy: require-corp` headers so that `SharedArrayBuffer` works for pthreads. The built-in `npm run serve` already does this.

### 3. Codespell
A `.codespellrc` is present for spell-checking. It skips `./src/3rdparty`, `./src/crypto/ghostrider`, and several other large/generated files.

---

## Code Style & Conventions

### Naming
- **Classes:** PascalCase (`VirtualMemory`, `RxQueue`, `WebGpuBackend`)
- **Methods / variables:** camelCase (`backgroundInit()`, `m_worker`)
- **Member variables:** `m_` prefix (`m_state`, `m_mutex`)
- **Macros / constants:** `SCREAMING_SNAKE_CASE` (`XMRIG_MINER_PROJECT`, `APP_VERSION`)

### File organization
- Every `.cpp` has a matching `.h` in the same or parent directory.
- Platform-specific implementations use filename suffixes:
  - `_unix`, `_win`, `_wasm`, `_linux`, `_mac`, `_hwloc`
- Example: `Signals.cpp` vs `Signals_wasm.cpp`, `VirtualMemory_unix.cpp` vs `VirtualMemory_wasm.cpp`

### CMake patterns
- Source lists are defined in per-module `.cmake` files (`base.cmake`, `backend.cmake`, `cpu.cmake`, etc.) and included by the root `CMakeLists.txt`.
- Conditional sources use either:
  - `list(APPEND SOURCES_OS src/App_wasm.cpp)`
  - `$<IF:$<BOOL:${EMSCRIPTEN}>,src/base/io/Signals_wasm.cpp,src/base/io/Signals.cpp>`

### Preprocessor guards
- Use `#ifdef __EMSCRIPTEN__` or `#ifdef XMRIG_OS_WASM` for WASM-specific code paths.
- Algorithm families are toggled with `XMRIG_ALGO_CN_LITE`, `XMRIG_ALGO_CN_HEAVY`, etc.

### Logging
Use the macros from `base/io/log/Log.h`:
- `LOG_VERBOSE("...")`
- `LOG_WARN("%s" YELLOW_BOLD_S "...", Tags::randomx())`
- `LOG_ERR("...")`

### License headers
All source files carry a GPL-3.0-or-later header. New files should preserve this style.

---

## Security Considerations

1. **Hardcoded credentials in `deploy.py`**  
   The deployment script contains a hardcoded SFTP password. This is a known security risk and must not be committed to public repositories without remediation.

2. **Cryptocurrency miner nature**  
   XMRig is a miner. Running it in a browser context (WASM) touches on cryptojacking concerns. Any deployment must comply with local laws and user-consent requirements.

3. **No JIT in WASM**  
   Browsers enforce a W^X (write-xor-execute) memory model. The RandomX JIT compiler cannot run in WASM; the build falls back to the **interpreter-only** path. This is functionally correct but extremely slow.

4. **No memory protection APIs**  
   `mmap`/`mprotect`/`mlock` are unavailable. The WASM shim uses plain `malloc` with a recycling pool. Do not assume hardware-enforced page protections exist.

5. **WebGPU backend is incomplete**  
   The `EM_JS` bridge functions in `src/wasm/webgpu_js.cpp` are stubs returning `-1`. There is no active GPU compute path yet.

---

## WASM Porting Notes

These are the specific architectural changes made to support Emscripten/WebAssembly. If you modify core code, you must preserve these shims.

### libuv Shim (`src/wasm/uv_wasm.cpp` + `src/wasm/uv.h`)
XMRig depends heavily on libuv for its event loop, timers, async callbacks, and networking. Because libuv is not fully available in the browser, a minimal compatible shim is provided:

- **`uv_run()`** — Does not return. Runs a `while (!g_uv_stop)` loop calling `tick_timers()` and `emscripten_thread_sleep(1.0)`. Mining pthreads do the actual work; the main thread stays alive to service timers.
- **Timers (`uv_timer_*`)** — Tracked in a global `std::vector<TimerEntry>` and driven by `emscripten_get_now()`.
- **Async (`uv_async_*`)** — Callbacks fire synchronously on the main thread.
- **Work queue (`uv_queue_work`)** — Runs synchronously; `work_cb` then `after_cb` are called immediately.
- **Network / FS / TTY** — Stubbed to return errors (`UV_ENOTSUP`, `UV_EINVAL`).
- **Memory reporting** — `uv_get_total_memory()` returns 512 MB as a rough heuristic.

### VirtualMemory (`src/crypto/common/VirtualMemory_wasm.cpp`)
Replaces POSIX `mmap`/`mprotect`/`mlock` with a `malloc`-based allocator plus per-size recycling pools:

| Pool size | Typical use |
|-----------|-------------|
| 2 MB      | CryptoNight / RandomX scratchpad |
| 16 MB     | RandomX cache (reduced) |
| 256 MB    | Large dataset blocks |

Memory protection functions (`protectRX`, `protectRW`, `protectRWX`) are no-ops.

### Signal Handling (`src/base/io/Signals_wasm.cpp`)
POSIX signals (`SIGTERM`, `SIGINT`, `SIGHUP`, `SIGUSR1`) do not exist in the browser. The WASM stub logs a verbose message and does nothing. Graceful shutdown must be triggered from JavaScript calling an exported C++ function.

### RandomX Light Mode
RandomX normally builds a ~2 GB dataset at startup. In WASM this is forced to **light mode**:
- `src/crypto/rx/RxDataset.cpp` — skips dataset allocation under `#ifdef __EMSCRIPTEN__`
- `src/crypto/rx/RxConfig.cpp` — forces light mode flag

This makes startup instant instead of multi-minute.

### RxQueue Deadlock Fix
A previous bug in `src/crypto/rx/RxQueue.cpp` caused a deadlock because `backgroundInit()` held `m_mutex` while calling `onReady()`, which tried to lock the same mutex again. The fix is to release the lock before the callback:

```cpp
m_state = STATE_IDLE;
lock.unlock();   // release before callback
onReady();
```

### Threading Model
- Uses **Emscripten pthreads** (`-s USE_PTHREADS=1`).
- Pool size: **16** workers (`PTHREAD_POOL_SIZE=16`).
- CPU mining workers run on pthreads.
- The main thread runs the event loop (`uv_run`) and must not block for long periods.

### App Entry (`src/App_wasm.cpp`)
Replaces the native `fork()`/`setsid()` daemon background mode with a no-op stub that returns `false` (meaning "keep running in foreground").

---

## Known Issues & Limitations

1. **Extremely slow RandomX in WASM**  
   Interpreter-only RandomX in WASM achieves roughly **0.8 hashes/sec per thread**. A 1M hash benchmark would take roughly 14 days on a single thread. Treat the WASM build as a "does it start and hash correctly?" validator, not a performance tool.

2. **No native networking in WASM**  
   The stratum pool client uses raw TCP; this does not work in the browser. Pool mining is not functional in the WASM build without a WebSocket translation layer.

3. **WebGPU backend is not end-to-end**  
   - WGSL shaders for CryptoNight Variant 0 are written (`src/backend/webgpu/shaders/wgsl/cn/`).
   - C++ wrappers (`WebGpuDevice`, `WebGpuBuffer`, `WebGpuKernel`, etc.) exist.
   - The JavaScript bridge (`src/wasm/webgpu_js.cpp`) is stubbed. Real `navigator.gpu` calls are not yet implemented.

4. **GhostRider disabled**  
   GhostRider relies on x86 intrinsics and libuv threading patterns that are incompatible with Emscripten. It is force-disabled in `CMakeLists.txt`.

5. **File I/O unavailable**  
   File logging, config file watching, and TTY/console features are disabled or stubbed for WASM.

6. **No hardware introspection**  
   DMI/SMBIOS reading, CPU affinity, MSR mods, and NUMA awareness are all disabled for WASM.

---

## References

- `README.md` — Original XMRig README (human-oriented)
- `WASM_BUILD_AND_TEST.md` — Detailed Emscripten build and test guide
- `emscripten_porting_plan.md` — Original porting strategy document
- `webgpu_porting.md` — WebGPU/WGSL architecture and progress tracking
- `build_notes.md` — Short native build prerequisite notes
- `THREADING_AND_MEMORY_DESIGN.md` — Upstream design documentation
