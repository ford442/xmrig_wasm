/* libuv shim implementations for Emscripten/WASM builds.
 *
 * This file implements the non-trivial parts of the libuv shim declared in
 * uv.h. Key points:
 *
 *  - uv_run() schedules a repeating callback via emscripten_set_main_loop so
 *    that timers and async sends keep firing after exec() returns control to
 *    the JavaScript event loop.  emscripten_set_main_loop does not return.
 *
 *  - Timers are tracked in a global list and driven by the main loop
 *    callback so that hash-rate reporting and retry timers work.
 *
 *  - uv_close() calls the close callback synchronously (acceptable because
 *    no file descriptors or OS resources are involved in WASM).
 *
 *  - Network and file-system functions are stubs that always fail; the
 *    stratum pool connection and log file writing simply won't work until
 *    proper WebSocket and IndexedDB backends are added.
 */

#include "uv.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <atomic>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/threading.h>
#endif

/* -------------------------------------------------------------------------
 * Internal timer registry
 * ---------------------------------------------------------------------- */

namespace {

struct TimerEntry {
    uv_timer_t *handle;
    uint64_t    next_ms;   // absolute ms when the timer fires next
};

static std::vector<TimerEntry> g_timers;

static uint64_t now_ms()
{
#ifdef __EMSCRIPTEN__
    return (uint64_t)emscripten_get_now();
#else
    return 0;
#endif
}

} // anonymous namespace

static void tick_timers()
{
    uint64_t t = now_ms();
    for (auto &e : g_timers) {
        if (!e.handle || e.handle->is_closing || !e.handle->active) {
            continue;
        }
        if (t >= e.next_ms) {
            if (e.handle->repeat) {
                e.next_ms = t + e.handle->repeat;
            } else {
                e.handle->active = 0;
            }
            if (e.handle->timer_cb) {
                void *data = e.handle->data;
                void **vtable = nullptr;
                if (data) {
                    vtable = *(void***)data;
                }
                fprintf(stderr, "DEBUG tick_timers calling cb=%p handle=%p data=%p vtable=%p\n", (void*)(uintptr_t)e.handle->timer_cb, (void*)e.handle, data, (void*)vtable);
                e.handle->timer_cb(e.handle);
                fprintf(stderr, "DEBUG tick_timers cb returned\n");
            }
        }
    }
    // Purge dead entries periodically
    g_timers.erase(
        std::remove_if(g_timers.begin(), g_timers.end(),
            [](const TimerEntry &e) {
                return !e.handle || e.handle->is_closing || !e.handle->active;
            }),
        g_timers.end());
}

extern "C" {

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void xmrig_uv_tick()
{
    tick_timers();
}

} // extern "C"


/* -------------------------------------------------------------------------
 * Loop
 * ---------------------------------------------------------------------- */

static uv_loop_t g_default_loop;
static std::atomic<int> g_uv_stop{0};

uv_loop_t *uv_default_loop(void)
{
    return &g_default_loop;
}

int uv_run(uv_loop_t * /*loop*/, uv_run_mode /*mode*/)
{
#ifdef __EMSCRIPTEN__
    /* With pthreads, blocking the main thread is acceptable because worker
     * threads run the actual mining.  Use a sleep loop instead of
     * emscripten_set_main_loop to avoid a function-pointer-table bug in
     * Emscripten that causes signature mismatches under optimisation. */
    while (!g_uv_stop.load()) {
        tick_timers();
        emscripten_thread_sleep(1.0);
    }
#endif
    return 0;
}

void uv_loop_close(uv_loop_t * /*loop*/) {}
void uv_stop(uv_loop_t * /*loop*/) { g_uv_stop = 1; }

/* -------------------------------------------------------------------------
 * Version / error helpers
 * ---------------------------------------------------------------------- */

const char *uv_version_string(void)
{
    return "1.44.0-wasm";
}

const char *uv_strerror(int err)
{
    switch (err) {
    case UV_EOF:          return "end of file";
    case UV_ENOTSUP:      return "operation not supported";
    case UV_EINVAL:       return "invalid argument";
    case UV_ENOMEM:       return "not enough memory";
    case UV_ECONNREFUSED: return "connection refused";
    case UV_ECONNRESET:   return "connection reset by peer";
    case UV_ETIMEDOUT:    return "connection timed out";
    case UV_EPIPE:        return "broken pipe";
    case UV_ENOENT:       return "no such file or directory";
    case UV_ENOBUFS:      return "no buffer space available";
    case UV_EAI_NONAME:   return "host not found";
    default:              return "unknown error";
    }
}

const char *uv_err_name(int err)
{
    switch (err) {
    case UV_EOF:          return "EOF";
    case UV_ENOTSUP:      return "ENOTSUP";
    case UV_EINVAL:       return "EINVAL";
    case UV_ENOMEM:       return "ENOMEM";
    case UV_ECONNREFUSED: return "ECONNREFUSED";
    case UV_ECONNRESET:   return "ECONNRESET";
    case UV_ETIMEDOUT:    return "ETIMEDOUT";
    case UV_EPIPE:        return "EPIPE";
    case UV_ENOENT:       return "ENOENT";
    case UV_ENOBUFS:      return "ENOBUFS";
    case UV_EAI_NONAME:   return "EAI_NONAME";
    default:              return "UNKNOWN";
    }
}

int uv_translate_sys_error(int sys_errno)
{
    return -sys_errno;
}

/* -------------------------------------------------------------------------
 * Handle lifecycle
 * ---------------------------------------------------------------------- */

int uv_is_closing(const uv_handle_t *h)
{
    return (!h || h->is_closing) ? 1 : 0;
}

void uv_close(uv_handle_t *h, uv_close_cb cb)
{
    if (!h || h->is_closing) {
        return;
    }
    h->is_closing = 1;
    h->close_cb   = cb;
    /* Synchronous close: invoke callback immediately.
     * This is safe because no OS file descriptors are in play in WASM. */
    if (cb) {
        cb(h);
    }
}

int uv_is_readable(const uv_stream_t * /*s*/)
{
    return 0; /* TTY not readable in browser */
}

int uv_is_writable(const uv_stream_t * /*s*/)
{
    return 0;
}

/* -------------------------------------------------------------------------
 * Timer
 * ---------------------------------------------------------------------- */

int uv_timer_init(uv_loop_t * /*loop*/, uv_timer_t *h)
{
    void *data = h->data;
    memset(h, 0, sizeof(*h));
    h->type = UV_TIMER;
    h->data = data;
    return 0;
}

int uv_timer_start(uv_timer_t *h, uv_timer_cb cb, uint64_t timeout, uint64_t repeat)
{
    if (!h) return UV_EINVAL;

    /* Remove any existing entry for this handle */
    g_timers.erase(
        std::remove_if(g_timers.begin(), g_timers.end(),
            [h](const TimerEntry &e) { return e.handle == h; }),
        g_timers.end());

    h->timer_cb = cb;
    h->timeout  = timeout;
    h->repeat   = repeat;
    h->active   = 1;

    TimerEntry e;
    e.handle  = h;
    e.next_ms = now_ms() + timeout;
    g_timers.push_back(e);
    return 0;
}

int uv_timer_stop(uv_timer_t *h)
{
    if (!h) return UV_EINVAL;
    h->active = 0;
    return 0;
}

uint64_t uv_timer_get_repeat(const uv_timer_t *h)
{
    return h ? h->repeat : 0;
}

void uv_timer_set_repeat(uv_timer_t *h, uint64_t repeat)
{
    if (h) h->repeat = repeat;
}

/* -------------------------------------------------------------------------
 * Async
 * ---------------------------------------------------------------------- */

int uv_async_init(uv_loop_t * /*loop*/, uv_async_t *h, uv_async_cb cb)
{
    if (!h) return UV_EINVAL;
    void *data = h->data;
    memset(h, 0, sizeof(*h));
    h->type     = UV_ASYNC;
    h->async_cb = cb;
    h->data     = data;
    return 0;
}

int uv_async_send(uv_async_t *h)
{
    if (!h || h->is_closing) return UV_EINVAL;
    /* Call synchronously since we're on the main WASM thread */
    if (h->async_cb) {
        h->async_cb(h);
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * Work queue  (synchronous; threading support added in a later phase)
 * ---------------------------------------------------------------------- */

int uv_queue_work(uv_loop_t * /*loop*/, uv_work_t *req,
                  uv_work_cb work_cb, uv_after_work_cb after_cb)
{
    if (!req) return UV_EINVAL;
    if (work_cb)       work_cb(req);
    if (after_cb)      after_cb(req, 0);
    return 0;
}

int uv_cancel(uv_work_t * /*req*/)
{
    return UV_ENOTSUP;
}

/* -------------------------------------------------------------------------
 * OS helpers
 * ---------------------------------------------------------------------- */

uint64_t uv_get_free_memory(void)
{
    /* Approximate: 256 MB free in a 512 MB WASM heap */
    return 256ULL * 1024 * 1024;
}

uint64_t uv_get_total_memory(void)
{
    return 512ULL * 1024 * 1024;
}

int uv_os_getpid(void)  { return 1; }
int uv_os_getppid(void) { return 0; }

static int copy_to_buf(const char *src, char *buf, size_t *size)
{
    size_t len = strlen(src);
    if (*size < len + 1) { *size = len + 1; return UV_ENOBUFS; }
    memcpy(buf, src, len + 1);
    *size = len;
    return 0;
}

int uv_os_homedir(char *buf, size_t *size) { return copy_to_buf("/", buf, size); }
int uv_os_tmpdir(char *buf, size_t *size)  { return copy_to_buf("/tmp", buf, size); }
int uv_exepath(char *buf, size_t *size)    { return copy_to_buf("/xmrig", buf, size); }
int uv_cwd(char *buf, size_t *size)        { return copy_to_buf("/", buf, size); }
int uv_chdir(const char * /*dir*/)         { return 0; }

int uv_getenv(const char *name, char *buf, size_t *size)
{
    const char *val = getenv(name);
    if (!val) return UV_ENOENT;
    return copy_to_buf(val, buf, size);
}

int uv_os_getenv(const char *name, char *buf, size_t *size)
{
    return uv_getenv(name, buf, size);
}
