# Build Notes

Before building the project, run:

```bash
sudo apt update
sudo apt install libhwloc-dev libuv1-dev
```

## Emscripten guide

To port this project to Emscripten/WASM, the following areas need dedicated changes:

- Replace `fork()`/`setsid()` with Emscripten-compatible threading or runtime control.
- Adapt `mmap`, `mprotect`, and executable memory allocation to use the WASM heap model instead of native mmap pages.
- Adjust or replace `libuv` networking and event loop code for Emscripten compatibility.
- Disable or avoid native OpenSSL and use browser/Emscripten-friendly TLS options if needed.
- Customize threading logic for Emscripten, including `std::thread`/`pthread` use and worker thread behavior.
- Review and adapt `mlock`, hugepage, and other native memory management calls.
- Do not attempt the CUDA plugin for Emscripten; focus on CPU/OpenCL and native browser-friendly code paths.

If OpenCL CPU support is desired, prioritize that path and avoid native CUDA/OpenCL GPU plugins.
