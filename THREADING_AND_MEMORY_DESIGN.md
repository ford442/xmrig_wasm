# Detailed Design: Threading & Memory Management for WASM

## 1. Hybrid Threading Architecture

### 1.1 Architecture Overview
```
┌─────────────────────────────────────────────────────────┐
│         Browser Main Thread (JavaScript)                │
│  ┌──────────────────────────────────────────────────┐   │
│  │  Event Loop / UI Interaction / Configuration     │   │
│  └──────────────────────────────────────────────────┘   │
└────────────┬────────────────────────────────────────────┘
             │
      ┌──────┴──────┐
      │             │
      ▼             ▼
┌──────────────┐ ┌──────────────┐
│  Worker 1    │ │  Worker 2    │  (Multiple Workers)
│  RandomX     │ │  CryptoNight │
│  Cache Init  │ │  Mining Loop │
│  Algorithm   │ │  Pool Comms  │
│  Warmup      │ │  (if needed) │
└──────────────┘ └──────────────┘
```

### 1.2 Work Distribution
**Main Thread Responsibilities:**
- Configuration management
- User input handling
- Results aggregation and display
- Worker lifecycle management
- Graceful shutdown

**Worker Thread Responsibilities:**
- **Worker 1: Cache/Initialization**
  - RandomX cache initialization (time-consuming)
  - Algorithm warmup and verification
  - Memory allocation
  - Signals: Ready → Main thread

- **Worker 2+: Mining**
  - Actual mining loop (hash computation)
  - Multiple workers for CPU count on WASM
  - Signals: Hash results → Main thread
  - Periodic health checks

- **Worker N: Network** (Optional)
  - Pool communication via WebSocket
  - Job distribution from pool
  - Share submission

### 1.3 Communication Protocol

**Main Thread ↔ Worker Message Format:**
```javascript
// Main → Worker
{
  type: "init",
  algorithm: "randomx",      // Algorithm name
  config: {...},              // Algorithm config
  memorySize: 2097152,        // Size in bytes
  sharedBuffer: ArrayBuffer   // Shared memory (if applicable)
}

// Worker → Main (Results)
{
  type: "status",
  status: "ready" | "mining" | "error",
  message: string,
  hashCount?: number,
  hashRate?: number
}

{
  type: "hash",
  hash: Uint8Array,
  nonce: number
}
```

### 1.4 Implementation Approach

**C++ Side:**
1. Create `src/wasm/WasmWorker.h` - Base worker class
2. Create `src/wasm/WasmWorkerPool.cpp` - Manage worker creation/lifecycle
3. Create `src/wasm/WasmMessage.h` - Message serialization
4. Modify `src/App.cpp` - Event loop for main thread (simplified)

**JavaScript Side:**
1. Create `wasm/worker_init.js` - Worker initialization
2. Create `wasm/worker_pool.js` - Pool management
3. Create `wasm/message.js` - Message handling
4. Create `wasm/main.js` - Main thread orchestration

### 1.5 Emscripten PTHREAD Mapping
Emscripten can auto-map `pthread_create()` to Web Workers:
- Set `PTHREAD_POOL_SIZE` build flag
- Use `-s USE_PTHREADS=1` compiler flag
- Allows using std::thread directly (Emscripten handles Worker creation)

**Alternative: Manual Worker Binding**
- Create Emscripten EM_JS functions for worker control
- Export C++ functions callable from JavaScript
- Wire worker messages to C++ callbacks via `EM_ASM`

**Recommendation:** Use Emscripten PTHREAD mapping if possible (simpler), fall back to manual if performance requires optimization.

---

## 2. Custom Memory Allocator (Pool-Based)

### 2.1 Fixed-Size Pool Design

**Pool Strategy:**
Pre-allocate pools for common allocation sizes to reduce malloc/free churn:
```
Pool Structure:
┌─────────────────────────────────────────┐
│ Pool Manager                             │
├──────────────────┬──────────────────────┤
│ Pool 2MB         │ [  ] [  ] [  ]       │ (16 blocks)
│ Pool 16MB        │ [  ] [  ]            │ (4 blocks)
│ Pool 256MB       │ [  ]                 │ (2 blocks)
│ Fallback malloc  │ General allocator    │
└──────────────────┴──────────────────────┘
```

### 2.2 Pool Sizes and Pre-allocation

| Pool Size | Purpose | Count | Total Memory |
|-----------|---------|-------|--------------|
| 2 MB | RandomX small, CryptoNight | 16 | 32 MB |
| 16 MB | RandomX cache (reduced) | 4 | 64 MB |
| 256 MB | Large cache blocks | 2 | 512 MB |
| Fallback | Other allocations | - | On-demand |

**Configurable via CMake:**
```cmake
set(XMRIG_WASM_POOL_2MB_SIZE 16 CACHE STRING "Number of 2MB pools")
set(XMRIG_WASM_POOL_16MB_SIZE 4 CACHE STRING "Number of 16MB pools")
set(XMRIG_WASM_TOTAL_MEMORY 512 CACHE STRING "Total WASM memory in MB")
```

### 2.3 Allocator API (Replaces VirtualMemory)

**Header: `src/crypto/common/VirtualMemory_wasm.h`**
```cpp
class WasmMemoryAllocator {
public:
    // Main allocation functions
    static void* allocateLargePagesMemory(size_t size);
    static void* allocateOneGbPagesMemory(size_t size);
    static void releaseLargePagesMemory(void* ptr, size_t size);

    // Memory protection (no-op)
    static void protectRW(void* ptr, size_t size);
    static void protectRWX(void* ptr, size_t size);
    static void protectRX(void* ptr, size_t size);

    // Memory locking (no-op)
    static void lock(void* ptr, size_t size);
    static void unlock(void* ptr, size_t size);

    // Pool statistics
    static void printStats();
    static void reset();
};
```

### 2.4 Implementation Strategy

**File: `src/crypto/common/VirtualMemory_wasm.cpp`**
```cpp
class PoolAllocator {
private:
    struct Pool {
        size_t blockSize;
        std::vector<uint8_t*> blocks;
        std::vector<bool> inUse;
        // ... pool management
    };

    std::map<size_t, Pool> pools;
    std::mutex poolMutex;

public:
    void* allocate(size_t size) {
        // Try to find matching pool
        // If found and available, return block
        // Otherwise, use malloc fallback
    }

    void release(void* ptr, size_t size) {
        // Find pool containing ptr
        // Mark block as available
        // If not from pool, free via malloc
    }
};
```

### 2.5 Memory Layout Diagram

```
WASM Linear Memory (e.g., 512 MB)
┌──────────────────────────────────────────────┐
│ Static Data / Code (.data, .bss, code)       │ ~50 MB
├──────────────────────────────────────────────┤
│ Malloc Heap (standard allocator)             │ ~100 MB
├──────────────────────────────────────────────┤
│ Pool 2MB Pool [32 MB total]                  │ 32 MB
│  Block 0 [ ]  Block 1 [ ]  ... Block 15 [ ] │
├──────────────────────────────────────────────┤
│ Pool 16MB Pool [64 MB total]                 │ 64 MB
│  Block 0 [ ]  Block 1 [ ]  Block 2 [ ] Block 3 [ ] │
├──────────────────────────────────────────────┤
│ Pool 256MB Pool [512 MB total]               │ 512 MB
│  Block 0 [ ]  Block 1 [ ]                    │
├──────────────────────────────────────────────┤
│ Stack (grows downward)                       │ ~10 MB
└──────────────────────────────────────────────┘
```

### 2.6 Configuration in CMakeLists.txt

```cmake
if(XMRIG_WASM)
    # Use WASM-specific memory allocator
    list(APPEND SOURCES src/crypto/common/VirtualMemory_wasm.cpp)
    
    # Pool configuration
    add_definitions(
        -DWASM_POOL_2MB_SIZE=16
        -DWASM_POOL_16MB_SIZE=4
        -DWASM_TOTAL_MEMORY=512
    )
    
    # Emscripten-specific flags
    set(EMSCRIPTEN_FLAGS
        "-s ALLOW_MEMORY_GROWTH=1"
        "-s INITIAL_MEMORY=134217728"  # 128 MB initial
        "-s MAXIMUM_MEMORY=536870912"  # 512 MB max
        "-s WASM=1"
    )
endif()
```

---

## 3. Conditional JIT with Browser Feature Detection

### 3.1 JIT Capability Detection

**Browser Features Checked:**
1. **SharedArrayBuffer** - Available in modern browsers (indicates security posture)
2. **WebAssembly.Memory** - Growable memory support
3. **WebAssembly.Instance** - Module instantiation capability
4. **Dynamic code generation** - Can we allocate executable memory?

**Detection Function (JavaScript):**
```javascript
function detectWasmCapabilities() {
    return {
        hasSharedArrayBuffer: typeof SharedArrayBuffer !== 'undefined',
        hasWasmMemory: typeof WebAssembly.Memory !== 'undefined',
        supportsGrowableMemory: WebAssembly.Memory.toString().includes('grow'),
        browserJit: typeof JIT !== 'undefined',  // Browser-specific JIT
        canAllocateExecMemory: testExecMemory()  // Try-catch test
    };
}

function testExecMemory() {
    try {
        // Attempt to allocate memory and test exec behavior
        // Return true if successful
        return true;
    } catch (e) {
        return false;
    }
}
```

### 3.2 JIT Fallback Strategy

```
┌─────────────────────────────────────┐
│  Algorithm Initialization           │
├─────────────────────────────────────┤
│  Check Browser Capabilities         │
├─────────────────────────────────────┤
│  hasSharedArrayBuffer? && WASM?     │
│         /        \                  │
│        Y          N                 │
│       /            \                │
│  Try JIT        Use Interpreter     │
│   /    \         (Fallback)         │
│  Y      N                           │
│ /        \                          │
│JIT      Interpreter                 │
│Code     Code                        │
└─────────────────────────────────────┘
```

### 3.3 Implementation Details

**Header: `src/crypto/common/JitCapability.h`**
```cpp
class JitCapability {
public:
    static bool canUseJit();
    static bool supportsExecMemory();
    static bool hasSharedMemory();
    static JitMode getRecommendedMode();  // JIT, INTERPRETER, or AUTO
};
```

**File: `src/crypto/randomx/jit_compiler_wasm.cpp`**
```cpp
class JitCompilerWasm {
public:
    JitCompilerWasm() {
        if (JitCapability::canUseJit()) {
            m_useJit = true;
        } else {
            m_useJit = false;
            // Fall back to interpreter
        }
    }
    
    void compile(const uint8_t* code) {
        if (m_useJit) {
            compileToNativeCode(code);
        } else {
            compileToInterpreter(code);
        }
    }
};
```

### 3.4 Memory Management for Executable Code

**Option A: Writable-Writable (W^W)**
- Keep JIT memory writable throughout
- Trust browser sandbox
- Simpler implementation

**Option B: Copy-on-Execute**
- Generate code in writable buffer
- Copy to separate execute buffer (read-only)
- Requires instruction cache flush

**Option C: Interpreter with Translation**
- Use interpreter by default
- Cache translated blocks in pool
- Gradually move hot paths to pseudo-JIT

**Recommendation:** Option A (W^W) for initial WASM, with fallback to Interpreter if performance insufficient.

---

## 4. Integration Points

### 4.1 File Organization
```
src/
├── crypto/
│   ├── common/
│   │   ├── VirtualMemory_wasm.cpp       (NEW - Pool allocator)
│   │   ├── VirtualMemory_wasm.h         (NEW)
│   │   └── JitCapability.cpp            (NEW)
│   └── randomx/
│       └── jit_compiler_wasm.cpp        (NEW - Conditional JIT)
├── wasm/                                 (NEW directory)
│   ├── WasmWorker.h                     (Worker base class)
│   ├── WasmWorkerPool.cpp               (Pool management)
│   ├── WasmMessage.h                    (Message serialization)
│   └── wasm_main.cpp                    (Entry point)
├── App.cpp                              (MODIFY - Event loop)
└── backend/
    └── common/
        └── Thread.h                     (MODIFY - Add WASM stubs)
```

### 4.2 Build Configuration
```cmake
# Top-level CMakeLists.txt
if(XMRIG_WASM)
    # Custom memory allocator
    list(APPEND SOURCES src/crypto/common/VirtualMemory_wasm.cpp)
    
    # JIT capability detection
    list(APPEND SOURCES src/crypto/common/JitCapability.cpp)
    
    # WASM worker infrastructure
    file(GLOB WASM_SOURCES "src/wasm/*.cpp")
    list(APPEND SOURCES ${WASM_SOURCES})
    
    # Emscripten settings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_PTHREADS=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s PTHREAD_POOL_SIZE=4")
endif()
```

### 4.3 JavaScript Glue Code Structure
```
wasm/
├── index.html               (Main page)
├── worker_init.js          (Worker setup)
├── worker_pool.js          (Pool management)
├── message.js              (Protocol)
├── xmrig_module.js         (Emscripten wrapper)
└── main.js                 (Orchestration)
```

---

## 5. Testing Strategy

### 5.1 Unit Tests (C++)
```cpp
// Test pool allocator
TEST(WasmAllocator, AllocateAndDeallocate) {
    void* ptr = WasmMemoryAllocator::allocateLargePagesMemory(2 * 1024 * 1024);
    ASSERT_NE(nullptr, ptr);
    WasmMemoryAllocator::releaseLargePagesMemory(ptr, 2 * 1024 * 1024);
}

// Test JIT capability detection
TEST(JitCapability, CanDetectSupport) {
    bool canUse = JitCapability::canUseJit();
    ASSERT_TRUE(canUse || !canUse);  // Should not crash
}
```

### 5.2 Integration Tests (JavaScript)
```javascript
// Test worker initialization
test('Worker initializes correctly', async () => {
    const worker = new Worker('worker_init.js');
    worker.postMessage({type: 'init', algorithm: 'randomx'});
    const response = await new Promise(r => worker.onmessage = r);
    expect(response.data.status).toBe('ready');
});

// Test memory allocation
test('Memory pools allocate correctly', () => {
    const module = Module;
    const ptr = module._allocate_wasm_memory(2 * 1024 * 1024);
    expect(ptr).toBeGreaterThan(0);
});
```

### 5.3 Performance Benchmarks
- Hash rate comparison: WASM vs Native
- Memory usage: Pool efficiency
- Worker overhead: Communication latency
- Algorithm correctness: Hash output verification

---

## 6. Success Criteria

✅ **Memory Allocator:**
- Fixed-size pools allocate correctly
- No malloc/free churn for common sizes
- Memory overhead < 10%
- No memory leaks over 1000 allocations/deallocations

✅ **Threading:**
- Workers initialize without errors
- Multiple workers run in parallel
- Message passing works bidirectionally
- No deadlocks between main and workers

✅ **JIT:**
- Capability detection works (no crash)
- JIT compiles and executes correctly
- Interpreter fallback produces same hash results
- Mode selection transparent to algorithm layer

---

## Implementation Order (Recommended)

1. **Virtual Memory** → Build and test allocator first (foundation)
2. **JIT Detection** → Add capability detection to algorithm layer
3. **Threading** → Wire up worker communication
4. **Integration** → Connect all pieces in App.cpp event loop
5. **Testing** → Verify correctness and performance
