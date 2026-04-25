Summary: Benchmark hang is fixed
  What was actually happening
  The "hang" was not a single bug but a cascade of three issues:
   Issue                                 Symptom                                         Root cause
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   1. Dataset init never completed       callMain sat forever with no worker output      RandomX full-dataset init (~34 M items, ~2.2 GB) runs
                                                                                         through a superscalar interpreter in WASM. Without x86
                                                                                         JIT compilation this is ~100× slower than native — ~38
                                                                                         min single-threaded, ~5 min with 8 threads. It looked
                                                                                         like a hang because no visible progress was made in te
                                                                                         st timeouts.
   2. RxQueue::backgroundInit deadlock   After dataset init finished, nothing happened   backgroundInit() held m_mutex while calling onReady(),
                                                                                         but onReady() immediately tried to lock the same std::
                                                                                         mutex again. On Emscripten pthreads this deadlocked de
                                                                                         terministically.
   3. Async signal from workers lost     BenchState::done() never stopped the loop       BenchState::done() runs on worker pthreads. Our earlie
                                                                                         r uv_async_send() shim called callbacks synchronously,
                                                                                         which raced/crashed. This was already fixed in a previ
                                                                                         ous iteration with the g_pending_async queue + tick_as
                                                                                         ync() drain in uv_run().
  Fixes applied
  1. Force RandomX light mode for WASM
  Files: src/crypto/rx/RxDataset.cpp, src/crypto/rx/RxConfig.cpp
  // In RxDataset::allocate()
  #ifdef __EMSCRIPTEN__
      LOG_WARN("%s" YELLOW_BOLD_S "WASM build: forcing RandomX light mode", Tags::randomx());
      return;   // skip 2 GB dataset allocation
  #endif
  Light mode skips the 34 M-item dataset generation entirely; the VM computes dataset items on-the-fly from the ~256 KB cache. This makes startu
  p instant instead of multi-minute.
  2. Fix mutex deadlock in RxQueue::backgroundInit
  File: src/crypto/rx/RxQueue.cpp
  // BEFORE (deadlock)
  lock.lock();
  m_state = STATE_IDLE;
  onReady();          // onReady() also does lock(m_mutex) → deadlock

  // AFTER
  m_state = STATE_IDLE;
  lock.unlock();      // release before callback
  onReady();
  3. Increase pthread pool size
  File: cmake/flags.cmake
  -s PTHREAD_POOL_SIZE=16   # was 4
  This ensures enough workers are available for both the background init thread and mining workers without on-demand creation stalls.
  4. Clean up debug instrumentation
  Removed ~100 fprintf(stderr, "DEBUG ...") statements that were added during the investigation.
  Verification
  With these fixes the benchmark starts successfully:
  Runtime initialized, calling callMain...
  # (no crash, no early return, process keeps running)
  We verified with instrumentation that:
  1. RxQueue::backgroundInit completes cache init in light mode (~instant)
  2. onReady() → Miner::onDatasetReady() → handleJobChange() fires correctly
  3. CpuWorker threads start and begin hashing
  Performance reality check
  In WASM without JIT, interpreted RandomX is very slow:
  • Light mode: ~0.8 hashes/sec per thread (measured)
  • A --bench=1M run would take roughly 14 days on a single thread
  This is inherent to running RandomX in WASM without a native JIT compiler. For practical use you should:
  • Use much smaller benchmark sizes for smoke-testing (the minimum valid size is 250K, which would still take ~3–4 days at 0.8 H/s)
  • Treat the WASM build as a "does it start and hash correctly?" validator, not a performance benchmark
  • If you need meaningful hashrate numbers, run native builds
  Files changed
  Key functional changes:
  • cmake/flags.cmake
  • src/crypto/rx/RxDataset.cpp
  • src/crypto/rx/RxConfig.cpp
  • src/crypto/rx/RxQueue.cpp
  • src/wasm/uv_wasm.cpp (cleanup of earlier debug prints)

  
  We've made substantial progress on the WebGPU porting plan. Here's a concise summary of what's been built:
  Infrastructure Complete:
  • src/backend/webgpu/ — Full backend mirroring the OpenCL architecture (Backend, Worker, LaunchData, Thread, wrappers, runners)
  • src/wasm/webgpu_js.cpp — EM_JS bridge stubs for the browser WebGPU API
  • CMake integration (WITH_WEBGPU option, auto-enabled for Emscripten)
  WGSL Shaders Complete:
  • cryptonight.wgsl — Full CryptoNight Variant 0 shader with cn0, cn1, cn2 entry points
  • keccak.wgsl + wolf-aes.wgsl — Supporting crypto primitives
  • mul_hi_u64 emulated in WGSL (no built-in available)
  Integration Done:
  • Nonce::WEBGPU backend enum added
  • WebGpuBackend registered in Miner.cpp
  • Porting notes added to all existing OpenCL/CUDA source files
  Next Step to Reach PoC: The JS bridge functions in src/wasm/webgpu_js.cpp are currently stubs returning -1. To get a working end-to-end hash i
  n the browser, we need to implement the actual JavaScript bodies that call navigator.gpu.requestAdapter() → requestDevice() → createShaderModu
  le() → createComputePipeline() → dispatchWorkgroups() → mapAsync() readback.
  Would you like me to:
  1. Implement the JS bridge with real WebGPU API calls?
  2. Update wasm_test.html to detect WebGPU and run a CN_0 benchmark?
  3. Verify the WGSL shader compiles in a browser by writing a minimal standalone test page?
