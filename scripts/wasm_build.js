#!/usr/bin/env node
'use strict';

/**
 * XMRig WASM build helper.
 *
 * Locates the Emscripten toolchain, then drives emcmake + emmake.
 *
 * Usage (called via npm scripts):
 *   node scripts/wasm_build.js configure
 *   node scripts/wasm_build.js build
 *   node scripts/wasm_build.js clean
 *   node scripts/wasm_build.js serve
 */

const { execSync, spawnSync } = require('child_process');
const path = require('path');
const fs = require('fs');
const os = require('os');

const ROOT     = path.resolve(__dirname, '..');
const BUILD    = path.join(ROOT, 'build_wasm');
const DIST     = path.join(ROOT, 'dist');
const SERVE_PORT = process.env.PORT || 8000;

/* ------------------------------------------------------------------
 * Emscripten discovery
 * ------------------------------------------------------------------ */

function findEmscripten() {
    // 1. Already on PATH?
    if (commandExists('emcmake')) {
        return null;  // null = "already activated, nothing to source"
    }

    // 2. EMSDK env var set?
    if (process.env.EMSDK) {
        const envScript = path.join(process.env.EMSDK, 'emsdk_env.sh');
        if (fs.existsSync(envScript)) return envScript;
    }

    // 3. Common installation paths
    const candidates = [
        path.join(os.homedir(), 'emsdk', 'emsdk_env.sh'),
        path.join(os.homedir(), 'dev', 'emsdk', 'emsdk_env.sh'),
        '/opt/emsdk/emsdk_env.sh',
        '/usr/local/emsdk/emsdk_env.sh',
        '/emsdk/emsdk_env.sh',
        path.join(ROOT, '..', 'emsdk', 'emsdk_env.sh'),
    ];

    for (const c of candidates) {
        if (fs.existsSync(c)) return c;
    }

    return false;  // false = "not found"
}

function commandExists(cmd) {
    try {
        execSync(`which ${cmd}`, { stdio: 'ignore' });
        return true;
    } catch { return false; }
}

/* Build a shell prefix that sources emsdk if needed */
function makeEnvPrefix(envScript) {
    if (envScript === null)  return '';   // already on PATH
    return `source "${envScript}" > /dev/null 2>&1 && `;
}

/* ------------------------------------------------------------------
 * Core build steps
 * ------------------------------------------------------------------ */

function configure(envPrefix) {
    fs.mkdirSync(BUILD, { recursive: true });

    const cmakeArgs = [
        '-DXMRIG_FEATURE_RANDOMX=ON',
        '-DXMRIG_FEATURE_CN=ON',
        `-DCMAKE_BUILD_TYPE=${process.env.BUILD_TYPE || 'Release'}`,
    ].join(' ');

    const cmd = `${envPrefix}emcmake cmake ${cmakeArgs} "${ROOT}"`;
    run(cmd, BUILD);
}

function build(envPrefix) {
    if (!fs.existsSync(path.join(BUILD, 'Makefile'))) {
        configure(envPrefix);
    }
    const jobs = process.env.JOBS || os.cpus().length;
    const cmd  = `${envPrefix}emmake make -j${jobs}`;
    run(cmd, BUILD);
    copyToDist();
}

function copyToDist() {
    fs.mkdirSync(DIST, { recursive: true });

    const files = [
        'xmrig.js',
        'xmrig.wasm',
        'wasm_test.html',
    ];

    for (const f of files) {
        const src = path.join(BUILD, f);
        const dst = path.join(DIST, f);
        if (fs.existsSync(src)) {
            fs.copyFileSync(src, dst);
            console.log(`Copied ${f} → dist/`);
        }
    }
}

function clean() {
    let removed = false;
    if (fs.existsSync(BUILD)) {
        fs.rmSync(BUILD, { recursive: true, force: true });
        console.log(`Removed ${BUILD}`);
        removed = true;
    }
    if (fs.existsSync(DIST)) {
        fs.rmSync(DIST, { recursive: true, force: true });
        console.log(`Removed ${DIST}`);
        removed = true;
    }
    if (!removed) {
        console.log('Nothing to clean.');
    }
}

function serve() {
    const serveDir = fs.existsSync(DIST) ? DIST : BUILD;
    if (!fs.existsSync(serveDir)) {
        console.error(`build_wasm/ does not exist. Run "npm run build" first.`);
        process.exit(1);
    }

    // Copy test harness if not already there
    const harnessSrc = path.join(ROOT, 'wasm_test.html');
    const harnessDst = path.join(serveDir, 'wasm_test.html');
    if (fs.existsSync(harnessSrc) && !fs.existsSync(harnessDst)) {
        fs.copyFileSync(harnessSrc, harnessDst);
        console.log(`Copied wasm_test.html to ${path.basename(serveDir)}/`);
    }

    // Use Node http module for zero-dep serving
    const http = require('http');
    const mime = {
        '.html': 'text/html',
        '.js':   'application/javascript',
        '.wasm': 'application/wasm',
        '.map':  'application/json',
        '.css':  'text/css',
    };

    const server = http.createServer((req, res) => {
        let urlPath = req.url === '/' ? '/wasm_test.html' : req.url;
        const file  = path.join(serveDir, urlPath.split('?')[0]);

        if (!file.startsWith(serveDir)) {
            res.writeHead(403);
            return res.end('Forbidden');
        }

        fs.readFile(file, (err, data) => {
            if (err) {
                res.writeHead(404);
                return res.end('Not found');
            }
            const ext = path.extname(file);
            // SharedArrayBuffer requires these headers
            res.writeHead(200, {
                'Content-Type':             mime[ext] || 'application/octet-stream',
                'Cross-Origin-Opener-Policy':   'same-origin',
                'Cross-Origin-Embedder-Policy': 'require-corp',
            });
            res.end(data);
        });
    });

    server.listen(SERVE_PORT, () => {
        const url = `http://localhost:${SERVE_PORT}/wasm_test.html`;
        console.log(`\n  XMRig WASM test harness:\n  ${url}\n`);
        console.log('  Press Ctrl+C to stop.\n');
        // Best-effort: open browser automatically
        const open = {
            linux:  `xdg-open "${url}" 2>/dev/null`,
            darwin: `open "${url}"`,
            win32:  `start "" "${url}"`,
        }[os.platform()];
        if (open) execSync(open, { stdio: 'ignore', shell: true });
    });
}

/* ------------------------------------------------------------------
 * Run helper
 * ------------------------------------------------------------------ */

function run(cmd, cwd) {
    console.log(`\n$ ${cmd.replace(/source.*?&&\s*/,'')}\n`);
    const result = spawnSync('bash', ['-c', cmd], {
        cwd,
        stdio: 'inherit',
        env: { ...process.env },
    });
    if (result.status !== 0) {
        process.exit(result.status || 1);
    }
}

/* ------------------------------------------------------------------
 * Entry point
 * ------------------------------------------------------------------ */

const action = process.argv[2] || 'build';

if (action === 'clean') {
    clean();
    process.exit(0);
}

if (action === 'serve') {
    serve();
    // serve() registers an http.Server — process stays alive
    return;
}

const envScript = findEmscripten();
if (envScript === false) {
    console.error([
        '',
        '  ERROR: Emscripten SDK not found.',
        '',
        '  Install it with:',
        '    git clone https://github.com/emscripten-core/emsdk.git',
        '    cd emsdk && ./emsdk install latest && ./emsdk activate latest',
        '    source emsdk/emsdk_env.sh',
        '',
        '  Or set the EMSDK environment variable to the emsdk root directory.',
        '',
    ].join('\n'));
    process.exit(1);
}

if (envScript) {
    console.log(`Using Emscripten from: ${path.dirname(envScript)}`);
}

const prefix = makeEnvPrefix(envScript);

if (action === 'configure') configure(prefix);
else if (action === 'build')  build(prefix);
else {
    console.error(`Unknown action: ${action}`);
    process.exit(1);
}
