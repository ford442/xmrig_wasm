# Emscripten/WASM Porting Plan for XMRig

## Overview
This document outlines the complete strategy for porting XMRig to Emscripten/WASM. XMRig is a high-performance CPU/GPU miner that relies heavily on platform-specific optimizations. Porting to WASM requires systematic replacement of incompatible APIs while preserving algorithm correctness.

## Key Constraints in WASM
1. **No Process Management**: No fork(), execve(), setsid(), or process spawning
2. **Unified Memory Model**: No mmap(), mprotect(), mlock() - just malloc/free
3. **No Native Threading**: No std::thread or pthread - use Web Workers or single-threaded
4. **No POSIX Signals**: No signal handlers, no sigaction()
5. **No File System**: No filesystem access unless via IndexedDB or virtual FS
6. **No Raw Sockets**: No TCP/UDP - use WebSocket or Fetch API
7. **No Direct Hardware Access**: No CPU affinity, MSR, DMI reading

## Phase 1: Infrastructure & Build System

### 1.1 Create Emscripten Toolchain Configuration
- [ ] Create `cmake/emscripten.cmake` or modify CMakeLists.txt to detect Emscripten
- [ ] Define `XMRIG_WASM` macro when `__EMSCRIPTEN__` is defined
- [ ] Create conditional compilation guards throughout codebase
- [ ] Test CMake can recognize emcripten cross-compiler

**Files to Create/Modify:**
- `CMakeLists.txt` - Add Emscripten detection
- New: `cmake/emscripten.cmake` - Emscripten-specific flags

### 1.2 Document Emscripten Integration Points
- [ ] Create `docs/EMSCRIPTEN_INTEGRATION.md` explaining callback model
- [ ] Document ASYNCIFY usage if needed for synchronous code
- [ ] Plan JavaScript glue code structure

**Files to Create:**
- `docs/EMSCRIPTEN_INTEGRATION.md`
- `src/wasm/` directory (new) for WASM-specific code

---

## Phase 2: POSIX Signal Handling (CRITICAL - Blocks Compilation)

### 2.1 Signals Replacement
**Current State:**
- `src/base/io/Signals.cpp` (88 lines) - Registers SIGPIPE, SIGUSR1 via libuv
- `src/crypto/rx/RxFix_linux.cpp` (71 lines) - SIGSEGV/SIGILL handlers with ucontext

**Strategy:**
1. Create signal stub layer: `src/base/io/Signals_wasm.cpp`
2. Implement `uv_signal_init()` and `uv_signal_start()` as no-ops for WASM
3. Replace exception handling in RxFix_linux.cpp with try-catch or bounds checking

**Files to Modify:**
- [ ] `src/base/io/Signals.h` - Add `#ifdef XMRIG_WASM` stubs
- [ ] `src/base/io/Signals.cpp` - Conditional WASM code path
- [ ] `src/crypto/rx/RxFix_linux.cpp` - Replace sigaction/ucontext with WASM-compatible error handling
- [ ] Create: `src/base/io/Signals_wasm.cpp` - WASM stub implementations

**Implementation Detail:**
```cpp
// Pseudo-code for Signals.h
#ifdef XMRIG_WASM
struct SignalsWasm {
    static int init() { return 0; }  // No-op
    static int start() { return 0; } // No-op
};
#endif
```

---

## Phase 3: Virtual Memory Management (CRITICAL - Core to Mining)

### 3.1 mmap/munmap Replacement
**Current State:**
- `src/crypto/common/VirtualMemory_unix.cpp` (296 lines)
- Uses: mmap, munmap, mprotect, mlock, munlock, madvise
- Hugepage support: MAP_HUGETLB, MAP_HUGE_2MB, MAP_HUGE_1G

**Strategy - Create WASM Variant:**
1. Create `src/crypto/common/VirtualMemory_wasm.cpp`
2. Implement allocation using malloc/new
3. Stub out memory protection (no-op functions)
4. Stub out hugepage operations

**Key Replacements:**
| Function | Native | WASM |
|----------|--------|------|
| allocateLargePagesMemory() | mmap + MAP_HUGETLB | malloc |
| allocateOneGbPagesMemory() | mmap + MAP_HUGE_1G | malloc |
| protectRX(addr, size) | mprotect(PROT_READ\|PROT_EXEC) | no-op |
| protectRW(addr, size) | mprotect(PROT_READ\|PROT_WRITE) | no-op |
| protectRWX(addr, size) | mprotect(PROT_READ\|PROT_WRITE\|PROT_EXEC) | no-op |
| mlock(addr, size) | mlock(addr, size) | no-op |
| munlock(addr, size) | munlock(addr, size) | no-op |

**Files to Create/Modify:**
- [ ] Create: `src/crypto/common/VirtualMemory_wasm.cpp` - malloc-based allocation
- [ ] Modify: `src/crypto/common/VirtualMemory.h` - Add WASM implementations
- [ ] Modify: `src/crypto/common/VirtualMemory.cpp` - Route WASM to new implementations
- [ ] Modify: `src/crypto/common/LinuxMemory.cpp` - Disable hugepage ops on WASM
- [ ] Modify: `CMakeLists.txt` - Include VirtualMemory_wasm.cpp for Emscripten builds

### 3.2 JIT Compiler Memory Handling (RandomX)
**Current State:**
- `src/crypto/randomx/jit_compiler_x86.cpp` - Uses protectRX for JIT pages
- `src/crypto/randomx/jit_compiler_a64.cpp` - ARM64 JIT memory
- `src/crypto/randomx/jit_compiler_rv64.cpp` - RISC-V JIT memory

**Challenge:** WASM doesn't support writable-executable memory (W^X security model)

**Strategy:**
1. For WASM, use writable memory during JIT generation
2. Keep memory writable throughout (trust browser's sandbox)
3. No page protection switches needed
4. May disable JIT entirely in favor of interpreter on WASM

**Files to Modify:**
- [ ] `src/crypto/randomx/jit_compiler_x86.cpp` - Add WASM-compatible memory handling
- [ ] `src/crypto/randomx/jit_compiler_a64.cpp` - Add WASM paths if needed
- [ ] `src/crypto/randomx/jit_compiler_rv64.cpp` - Add WASM paths if needed
- [ ] Consider: Create interpreter fallback that works better on WASM

### 3.3 Memory Pool & NUMA Handling
**Current State:**
- `src/crypto/common/MemoryPool.h/cpp` - Generic pooling
- `src/crypto/common/NUMAMemoryPool.h` - NUMA-aware pooling
- `src/crypto/rx/RxNUMAStorage.cpp` - NUMA cache allocation with threading

**Strategy:**
1. Disable NUMA code on WASM (no NUMA in browser)
2. Use regular MemoryPool for all allocations
3. Remove NUMA threading

**Files to Modify:**
- [ ] `src/crypto/common/NUMAMemoryPool.h` - Add WASM guard
- [ ] `src/crypto/rx/RxNUMAStorage.cpp` - Disable NUMA on WASM
- [ ] `CMakeLists.txt` - Skip NUMA compilation for Emscripten

---

## Phase 4: Threading Model (Algorithm Preservation)

### 4.1 Threading Strategy Decision
**Current State:**
- `src/backend/common/Thread.h` - std::thread wrapper with CPU affinity
- Workers create multiple threads for parallel mining
- Background threads: RxQueue, NUMA cache, GhostRider helper

**Options for WASM:**
1. **Single-threaded**: Remove threading, run mining loop on main thread
   - Simplest implementation
   - Lower performance but still functional
   - No Web Worker complexity

2. **Web Workers**: Use Emscripten's pthread_create → Web Worker mapping
   - Maintains parallelism
   - Requires worker thread communication
   - More complex but better performance

3. **Hybrid**: Single main thread + Web Workers for heavy background tasks
   - Balance of simplicity and performance

**Recommendation:** Start with **single-threaded** (Option 1) for initial WASM port. Can optimize to Web Workers later.

### 4.2 Single-Threaded Variant (Recommended)
**Files to Modify:**
- [ ] `src/backend/common/Thread.h` - Add WASM thread stub
- [ ] `src/backend/common/Workers.cpp` - Run workers on single thread sequentially or in event loop
- [ ] `src/crypto/rx/RxQueue.cpp/h` - Remove background thread, execute synchronously
- [ ] `src/crypto/rx/RxNUMAStorage.cpp` - Single-threaded initialization
- [ ] `src/crypto/ghostrider/ghostrider.cpp` - Remove HelperThread on WASM
- [ ] `src/hw/msr/Msr_win.cpp` - Remove thread on WASM

**Thread Stub Implementation:**
```cpp
// src/backend/common/Thread.h
#ifdef XMRIG_WASM
class Thread {
public:
    Thread(IWorker *worker) : m_worker(worker) {}
    void create() { m_worker->start(); }  // Run synchronously
    bool join() { return true; }
    // ... other no-op methods
};
#endif
```

### 4.3 Optional: Web Worker Variant (Future)
- [ ] Create `src/wasm/worker.cpp` - Worker entry point
- [ ] Create JavaScript glue for worker communication
- [ ] Implement work queue serialization

---

## Phase 5: Process Management & Hardware Info (Non-Critical)

### 5.1 Process Management Removal
**Current State:**
- `src/App_unix.cpp` - fork() + setsid() for background daemon
- `src/hw/msr/Msr_linux.cpp` - system("modprobe msr")

**Strategy:**
- [ ] `src/App_unix.cpp` - Remove fork/setsid, keep main loop
- [ ] `src/hw/msr/Msr_linux.cpp` - Disable on WASM
- [ ] Create conditional path for WASM app startup

### 5.2 Hardware Information (Skip on WASM)
**Current State:**
- `src/hw/dmi/DmiReader_unix.cpp` - DMI/SMBIOS reading
- `src/base/kernel/Platform_unix.cpp` - CPU affinity
- Various `BasicCpuInfo*.cpp` - Use std::thread::hardware_concurrency()

**Strategy:**
- [ ] Disable DMI reading on WASM
- [ ] Disable CPU affinity on WASM
- [ ] Keep hardware_concurrency() but return 1 on WASM
- [ ] Create `src/hw/dmi/DmiReader_wasm.cpp` - No-op implementation

---

## Phase 6: Event Loop & Networking

### 6.1 Event Loop Replacement (libuv → Emscripten)
**Current State:**
- `src/App.cpp:89` - `uv_run(uv_default_loop(), UV_RUN_DEFAULT)`
- libuv manages timers, async callbacks, TCP, DNS, file events

**Strategy:**
1. For initial WASM port: Remove/stub libuv event loop
2. Run mining loop synchronously or via Emscripten ASYNCIFY
3. Skip networking (no pool communication)
4. Disable file watching, console I/O

**Files to Modify:**
- [ ] `src/App.cpp` - Conditional WASM main loop (synchronous)
- [ ] `src/base/io/Async.cpp` - Stub async operations on WASM
- [ ] `src/base/tools/Timer.cpp` - Use JavaScript timers on WASM
- [ ] Skip for now: Signals, Console, Watcher, DnsUvBackend, HttpServer, etc.

### 6.2 Stratum Pool Connection (Optional)
**Current State:**
- `src/base/net/stratum/Client.cpp` - Pool communication

**Strategy:**
- [ ] For MVP: Disable pool communication on WASM
- [ ] Future: Implement WebSocket wrapper for stratum protocol
- [ ] Create `src/net/stratum/StratumWebSocket.cpp` (future phase)

### 6.3 Logging & Output
**Current State:**
- `src/base/io/log/FileLogWriter.cpp` - File logging
- `src/base/io/Console.cpp` - Terminal I/O

**Strategy:**
- [ ] Disable file logging on WASM
- [ ] Use console.log() for output
- [ ] Create `src/base/io/Console_wasm.cpp` - JavaScript console wrapper

---

## Phase 7: Compile & Verify

### 7.1 CMake Configuration
- [ ] Test: `emcmake cmake -B build_wasm -DCMAKE_BUILD_TYPE=Release`
- [ ] Verify: No undefined POSIX references
- [ ] Fix: Any remaining compilation errors
- [ ] Note: `CMAKE_PROJECT_NAME` may be rewritten to `xmrig-notls` when OpenSSL/TLS support is disabled, so avoid hardcoding `xmrig` target names in CMake post-build commands.
- [ ] Note: Disable GhostRider for WASM builds because it uses x86 intrinsics and libuv threading that are not compatible with Emscripten.
- [ ] Note: Activate the Emscripten environment before configuring. Run `source ./emsdk/emsdk_env.sh` or use `emcmake` from a shell where `emcc`/`em++` are on `PATH`.

### 7.2 Build
- [ ] Test: `cmake --build build_wasm -j4`
- [ ] Produce: `build_wasm/xmrig.js` and `build_wasm/xmrig.wasm`

### 7.3 Functional Testing
- [ ] Create `wasm_test.html` - Load WASM module
- [ ] Implement JavaScript API to start/stop mining
- [ ] Test: RandomX algorithm produces correct hashes
- [ ] Test: CryptoNight algorithm produces correct hashes
- [ ] Test: Mining loop runs without crashing

### 7.4 Performance Baseline
- [ ] Benchmark: Hash rate on simple test (1000 hashes)
- [ ] Compare: WASM vs native performance
- [ ] Identify: Any performance regressions

---

## Complete TODO List

### Critical Path (Must Complete)
- [ ] Phase 1.1: Emscripten CMake detection
- [ ] Phase 2.1: Signal handling stubs
- [ ] Phase 3.1: VirtualMemory_wasm.cpp with malloc
- [ ] Phase 4.2: Thread stubs for single-threaded
- [ ] Phase 5.1: Remove fork/setsid from App_unix.cpp
- [ ] Phase 6.1: Event loop replacement
- [ ] Phase 7.1-7.4: Compilation and testing

### Important (Should Complete)
- [ ] Phase 3.2: JIT compiler WASM support
- [ ] Phase 3.3: Disable NUMA on WASM
- [ ] Phase 5.2: Disable hardware info on WASM
- [ ] Phase 6.3: Logging via console.log()

### Optional (Nice to Have)
- [ ] Phase 1.2: Emscripten integration docs
- [ ] Phase 4.3: Web Workers (future optimization)
- [ ] Phase 6.2: WebSocket stratum protocol

---

## File Dependency Map

### Core Files Requiring Changes (In Dependency Order)
1. **CMakeLists.txt** - Enable Emscripten builds
2. **src/base/io/Signals.h/cpp** - Signal handling stubs
3. **src/crypto/common/VirtualMemory_wasm.cpp** - Memory allocation
4. **src/crypto/common/VirtualMemory.h** - Route to WASM impl
5. **src/backend/common/Thread.h** - Thread stubs
6. **src/App.cpp** - Event loop replacement
7. **src/crypto/rx/RxQueue.cpp** - Remove background thread
8. **src/hw/** - Disable hardware features
9. **src/base/io/** - Disable TTY, file watching, etc.

### Files to Create (New)
1. `src/crypto/common/VirtualMemory_wasm.cpp`
2. `src/base/io/Signals_wasm.cpp` (or inline in header)
3. `src/wasm/wasm_main.cpp` - WASM entry point (if needed)
4. `wasm_test.html` - Test harness
5. `emscripten_porting_plan.md` - This file

---

## Testing Checklist

### Compilation
- [ ] CMake configuration succeeds
- [ ] Emscripten compiler flags applied correctly
- [ ] No undefined symbol errors
- [ ] WASM binary generated successfully

### Runtime
- [ ] Module loads in browser without errors
- [ ] Main mining loop executes
- [ ] No crashes on first 100 hashes
- [ ] Output correct hash values for test inputs

### Algorithms
- [ ] RandomX produces correct hashes
- [ ] CryptoNight produces correct hashes
- [ ] GhostRider (if included) produces correct hashes
- [ ] KawPow (if included) produces correct hashes

### Performance
- [ ] Hash rate measurable (hashes/second)
- [ ] Memory usage reasonable
- [ ] No memory leaks detected
- [ ] Performance acceptable for browser use

---

## Known Limitations

1. **No GPU Mining**: CUDA and OpenCL plugins won't compile
2. **No Stratum Pool**: Network communication not supported in MVP
3. **No File I/O**: Config files must be provided via JavaScript
4. **Single-Threaded**: Initial version limited to one mining thread
5. **No Signal Handling**: No SIGTERM/SIGKILL support
6. **No Memory Protection**: JIT memory stays writable
7. **Limited Hardware Info**: Can't detect CPU features, NUMA, etc.

---

## Success Criteria

### MVP (Minimum Viable Product)
- ✅ WASM module compiles and links successfully
- ✅ Module loads in browser without errors
- ✅ Mining loop executes (at least 1000 hashes)
- ✅ Produces correct hash values for test inputs
- ✅ Single algorithm working (suggest RandomX)

### v1.0 (Production Ready)
- ✅ All algorithms working (RandomX, CryptoNight, GhostRider)
- ✅ Configurable via JavaScript API
- ✅ Graceful shutdown
- ✅ Performance within 50-70% of native version
- ✅ Comprehensive documentation

---

## References
- [Emscripten Documentation](https://emscripten.org/)
- [WASM Memory Model](https://webassembly.org/docs/spec/)
- XMRig build_notes.md - Original requirements
- XMRig source - CMakeLists.txt, src/App.cpp

