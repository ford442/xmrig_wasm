# XMRig WASM Build and Test Guide

This guide explains how to build XMRig for Emscripten/WebAssembly and test it using the provided HTML test harness.

## Prerequisites

### Install Emscripten SDK

The Emscripten toolchain is required to compile XMRig to WebAssembly.

```bash
# Clone the Emscripten SDK repository
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk

# Install the latest SDK
./emsdk install latest
./emsdk activate latest

# Source the environment (do this in every terminal session)
source ./emsdk_env.sh

# Verify installation
emcmake --version
emmake --version
```

### System Requirements

- Linux, macOS, or Windows (with WSL2)
- Python 3.6+
- Node.js 14+ (Emscripten includes Node, but a system version helps)
- CMake 3.16+
- Git

## Building XMRig for WASM

### Step 1: Navigate to the XMRig WASM Repository

```bash
cd /path/to/xmrig_wasm
```

### Step 2: Activate Emscripten Environment

Every build session requires activating the Emscripten environment:

```bash
source /path/to/emsdk/emsdk_env.sh
```

### Step 3: Create Build Directory

```bash
mkdir -p build_wasm
cd build_wasm
```

### Step 4: Configure with CMake

Use `emcmake` to wrap the standard `cmake` command:

```bash
emcmake cmake \
  -DXMRIG_FEATURE_RANDOMX=ON \
  -DXMRIG_FEATURE_CN=ON \
  -DXMRIG_FEATURE_CN_LITE=ON \
  -DCMAKE_BUILD_TYPE=Release \
  ..
```

**Configuration Options:**
- `-DXMRIG_FEATURE_RANDOMX=ON` - Enable RandomX algorithm (default: ON)
- `-DXMRIG_FEATURE_CN=ON` - Enable CryptoNight algorithm (default: ON)
- `-DXMRIG_FEATURE_CN_LITE=ON` - Enable CryptoNight Lite (default: ON)
- `-DXMRIG_FEATURE_CN_HEAVY=ON` - Enable CryptoNight Heavy (default: OFF)
- `-DCMAKE_BUILD_TYPE=Release` - Optimize for production (default: Release)
- `-DCMAKE_BUILD_TYPE=Debug` - Keep symbols for debugging

### Step 5: Build

Use `emmake` to wrap the standard `make` command:

```bash
emmake make -j$(nproc)
```

**Build Output:**

Upon successful completion, you should see:

```
[100%] Linking CXX executable xmrig
[100%] Built target xmrig
```

This generates three files in the `build_wasm/` directory:

```
xmrig.js       - Emscripten JavaScript wrapper (required runtime support)
xmrig.wasm     - WebAssembly binary (the actual compiled code)
xmrig.wasm.map - Source map for debugging (optional)
```

### Step 6: Verify Build Artifacts

```bash
ls -lh build_wasm/xmrig.js build_wasm/xmrig.wasm
```

Expected output (approximate sizes):
```
-rw-r--r--  1 user  group   50K Apr 19 10:30 xmrig.js
-rw-r--r--  1 user  group  2.5M Apr 19 10:30 xmrig.wasm
```

## Testing with HTML Harness

### Step 1: Serve the Build Artifacts

The WASM module must be served via HTTP (due to browser security restrictions).

#### Option A: Using Python (built-in)

```bash
cd build_wasm
python3 -m http.server 8000
```

#### Option B: Using Node.js http-server

```bash
npm install -g http-server
cd build_wasm
http-server
```

#### Option C: Using Node.js built-in server

```bash
cd build_wasm
node -e "const http = require('http'); http.createServer((req, res) => { const fs = require('fs'); const file = req.url === '/' ? '/wasm_test.html' : req.url; try { res.end(fs.readFileSync('.' + file)); } catch(e) { res.statusCode = 404; res.end('Not found'); } }).listen(8000);"
```

### Step 2: Copy Test Harness

The test harness needs to be in a location accessible to the web server:

```bash
# Option A: Copy to build_wasm directory
cp /path/to/xmrig_wasm/wasm_test.html build_wasm/

# Option B: Serve from parent directory
# (if serving from project root, wasm_test.html can auto-locate ../build_wasm/)
```

### Step 3: Open in Browser

Navigate to the test harness URL:

- **If harness is in build_wasm/:** `http://localhost:8000/wasm_test.html`
- **If harness is in project root:** `http://localhost:8000/wasm_test.html`

### Step 4: Run Test

1. **Select Algorithm** - Choose RandomX, CryptoNight, or CryptoNight Lite
2. **Configure Test Parameters:**
   - Test Duration: How long to run (seconds)
   - Hash Count: Target number of hashes to complete
   - Threads: Number of mining threads (1-8)
3. **Click "Start Test"** - Mining simulation begins
4. **Monitor Metrics:**
   - Hashes Completed: Total hashes computed
   - Hash Rate: Hashes per second
   - Elapsed Time: Test duration
   - Log Output: Detailed event log

## Understanding Build Output

### xmrig.js

The Emscripten-generated JavaScript wrapper that:
- Loads the WASM binary
- Manages Emscripten runtime (memory, modules, etc.)
- Provides interface between JavaScript and WASM
- Handles memory growth and I/O

### xmrig.wasm

The compiled WebAssembly binary containing:
- All C++ mining algorithms ported to WASM
- Memory-pooled allocators (no mmap/mprotect)
- Synchronous event loop (no threading in initial phase)
- Portable crypto implementations (no x86 SIMD)

### File Size Considerations

Typical WASM binary sizes:
- **Release build:** 2-3 MB (compressed with gzip: ~500-700 KB)
- **Debug build:** 10-15 MB (not recommended for deployment)

## Browser Compatibility

### Minimum Requirements

- **Chrome/Edge:** 57+
- **Firefox:** 52+
- **Safari:** 11+
- **Mobile browsers:** Most modern versions support WASM

### Required Browser Features

- WebAssembly (WASM)
- SharedArrayBuffer (for multi-threading, future phase)
- Web Workers (for threading, future phase)

## Troubleshooting

### Issue: "Failed to load WASM module"

**Symptoms:** Test harness shows "Could not find xmrig.js"

**Solutions:**
1. Verify build completed successfully: `ls -la build_wasm/xmrig.*`
2. Ensure WASM files are in the server's document root
3. Check server is running: `curl http://localhost:8000/xmrig.js | head -5`

### Issue: "Module is not a constructor"

**Symptoms:** Browser console shows JavaScript errors

**Solutions:**
1. Verify Emscripten version matches build: `emcmake --version`
2. Check `xmrig.js` isn't corrupted: `head -20 build_wasm/xmrig.js | grep "Module"`
3. Try rebuilding with fresh CMake cache: `rm -rf build_wasm && mkdir build_wasm && cd build_wasm && emcmake cmake .. && emmake make`

### Issue: "WASM module runtime initialized" but no mining

**Symptoms:** Log shows module loaded but stats don't update

**Solutions:**
1. Open browser DevTools (F12) and check Console for errors
2. Check if mining functions are exported properly
3. Verify algorithm selection matches compiled algorithms
4. Try single hash with immediate completion for debugging

### Issue: Build fails with "Command not found: emcmake"

**Symptoms:** `emcmake: command not found`

**Solutions:**
1. Ensure Emscripten environment is activated: `source /path/to/emsdk/emsdk_env.sh`
2. Verify emsdk installation: `ls /path/to/emsdk/emscripten/*/emcmake`
3. Update Emscripten: `cd /path/to/emsdk && git pull && ./emsdk install latest && ./emsdk activate latest`

### Issue: "Out of memory" during build

**Symptoms:** Build fails with memory errors

**Solutions:**
1. Increase Node.js memory limit: `emmake make --max-old-space-size=4096`
2. Reduce parallel jobs: `emmake make -j2` (instead of `-j$(nproc)`)
3. Use a machine with more available RAM

## Performance Expectations

### Hash Rate Estimates (Single Thread, Release Build)

| Algorithm | H/s | Notes |
|-----------|-----|-------|
| CryptoNight | 500-1000 | Portable C implementation |
| CryptoNight Lite | 1000-2000 | Lighter memory requirement |
| RandomX | 50-150 | Interpreter-only (no JIT in WASM) |

**Important:** These are rough estimates. Actual performance depends on:
- Browser/JavaScript engine (V8, SpiderMonkey, JavaScriptCore, etc.)
- CPU core performance
- System load
- WASM optimization level

## Advanced: Debugging

### Enable Debug Symbols

```bash
emcmake cmake -DCMAKE_BUILD_TYPE=Debug ..
emmake make
```

This generates `.wasm.map` files for browser debugging.

### View WASM in Browser DevTools

Most modern browsers (Chrome 63+, Firefox 79+) can inspect WASM:

1. Open DevTools (F12)
2. Go to Debugger/Sources tab
3. Look for WASM sections
4. Set breakpoints and inspect memory

### View Emscripten Runtime Logs

Add this to test harness JavaScript:

```javascript
Module.print = (text) => console.log('[WASM]', text);
Module.printErr = (text) => console.error('[WASM]', text);
```

## Next Steps

1. **Phase 5 (Pending):** Web Worker integration for true multi-threading
2. **WebSocket Support:** Connect to mining pools via browser WebSocket API
3. **IndexedDB Backend:** Persistent logging and statistics storage
4. **UI Dashboard:** Real-time graphs and pool integration UI

## References

- [Emscripten Documentation](https://emscripten.org/docs/)
- [WebAssembly MDN Guide](https://developer.mozilla.org/en-US/docs/WebAssembly)
- [XMRig GitHub](https://github.com/xmrig/xmrig)
- Build notes: See `build_notes.md` and `emscripten_porting_plan.md`
