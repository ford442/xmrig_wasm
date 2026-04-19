/* libuv compatibility shim for Emscripten/WASM builds.
 *
 * Provides the subset of the libuv API used by xmrig so that all source
 * files that include <uv.h> compile without modification.  None of the
 * network/IO functionality is implemented; mining algorithm code and the
 * timer/async plumbing that drives hash-rate reporting are the only
 * paths expected to function in the initial WASM port.
 *
 * uv_run() is implemented in uv_wasm.cpp using emscripten_set_main_loop.
 * All network and file-system functions are harmless no-ops that return
 * UV_ENOTSUP or 0 as appropriate.
 */

#ifndef UV_H
#define UV_H

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#else
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

/* -------------------------------------------------------------------------
 * Version
 * ---------------------------------------------------------------------- */
#define UV_VERSION_MAJOR 1
#define UV_VERSION_MINOR 44
#define UV_VERSION_PATCH 0

#ifdef __cplusplus
extern "C" {
#endif

const char *uv_version_string(void);

/* -------------------------------------------------------------------------
 * Error codes
 * ---------------------------------------------------------------------- */
#define UV__EOF       (-4095)
#define UV__ENOTSUP   (-4094)
#define UV__ENOBUFS   (-4093)
#define UV__EINVAL    (-22)
#define UV__ENOMEM    (-12)
#define UV__ECONNREFUSED (-111)
#define UV__ECONNRESET   (-104)
#define UV__ETIMEDOUT    (-110)
#define UV__EPIPE        (-32)
#define UV__ENOENT       (-2)
#define UV__EACCES       (-13)
#define UV__ENOBUFS      (-105)
#define UV__EAI_NONAME   (-3008)
#define UV__EAI_AGAIN    (-3001)
#define UV__EAI_FAIL     (-3002)

#define UV_EOF            UV__EOF
#define UV_ENOTSUP        UV__ENOTSUP
#define UV_EINVAL         UV__EINVAL
#define UV_ENOMEM         UV__ENOMEM
#define UV_ECONNREFUSED   UV__ECONNREFUSED
#define UV_ECONNRESET     UV__ECONNRESET
#define UV_ETIMEDOUT      UV__ETIMEDOUT
#define UV_EPIPE          UV__EPIPE
#define UV_ENOENT         UV__ENOENT
#define UV_EACCES         UV__EACCES
#define UV_ENOBUFS        UV__ENOBUFS
#define UV_EAI_NONAME     UV__EAI_NONAME
#define UV_EAI_AGAIN      UV__EAI_AGAIN
#define UV_EAI_FAIL       UV__EAI_FAIL

const char *uv_strerror(int err);
const char *uv_err_name(int err);
int         uv_translate_sys_error(int sys_errno);

/* -------------------------------------------------------------------------
 * Handle types and common fields
 * ---------------------------------------------------------------------- */

/* Every handle type must start with UV_HANDLE_FIELDS so that any handle
 * pointer can be safely cast to uv_handle_t *. */
#define UV_HANDLE_FIELDS  \
    void           *data; \
    int             type; \
    uv_close_cb     close_cb; \
    int             is_closing;

typedef enum {
    UV_UNKNOWN_HANDLE = 0,
    UV_ASYNC,
    UV_CHECK,
    UV_FS_EVENT,
    UV_FS_POLL,
    UV_HANDLE,
    UV_IDLE,
    UV_NAMED_PIPE,
    UV_POLL,
    UV_PREPARE,
    UV_PROCESS,
    UV_STREAM,
    UV_TCP,
    UV_TIMER,
    UV_TTY,
    UV_UDP,
    UV_SIGNAL,
    UV_FILE,
    UV_HANDLE_TYPE_MAX
} uv_handle_type;

/* Forward declarations for all handle/request types */
struct uv_loop_s;
struct uv_handle_s;
struct uv_stream_s;
struct uv_tcp_s;
struct uv_tty_s;
struct uv_timer_s;
struct uv_signal_s;
struct uv_async_s;
struct uv_poll_s;
struct uv_fs_event_s;
struct uv_write_s;
struct uv_connect_s;
struct uv_shutdown_s;
struct uv_getaddrinfo_s;
struct uv_work_s;
struct uv_fs_s;

typedef struct uv_loop_s         uv_loop_t;
typedef struct uv_handle_s       uv_handle_t;
typedef struct uv_stream_s       uv_stream_t;
typedef struct uv_tcp_s          uv_tcp_t;
typedef struct uv_tty_s          uv_tty_t;
typedef struct uv_timer_s        uv_timer_t;
typedef struct uv_signal_s       uv_signal_t;
typedef struct uv_async_s        uv_async_t;
typedef struct uv_poll_s         uv_poll_t;
typedef struct uv_fs_event_s     uv_fs_event_t;
typedef struct uv_write_s        uv_write_t;
typedef struct uv_connect_s      uv_connect_t;
typedef struct uv_shutdown_s     uv_shutdown_t;
typedef struct uv_getaddrinfo_s  uv_getaddrinfo_t;
typedef struct uv_work_s         uv_work_t;
typedef struct uv_fs_s           uv_fs_t;

/* Callback typedefs */
typedef void (*uv_close_cb)      (uv_handle_t *);
typedef void (*uv_timer_cb)      (uv_timer_t *);
typedef void (*uv_signal_cb)     (uv_signal_t *, int signum);
typedef void (*uv_async_cb)      (uv_async_t *);
typedef void (*uv_poll_cb)       (uv_poll_t *, int status, int events);
typedef void (*uv_alloc_cb)      (uv_handle_t *, size_t, struct uv_buf_t *);
typedef void (*uv_read_cb)       (uv_stream_t *, ssize_t, const struct uv_buf_t *);
typedef void (*uv_write_cb)      (uv_write_t *, int);
typedef void (*uv_connect_cb)    (uv_connect_t *, int);
typedef void (*uv_shutdown_cb)   (uv_shutdown_t *, int);
typedef void (*uv_fs_event_cb)   (uv_fs_event_t *, const char *, int, int);
typedef void (*uv_getaddrinfo_cb)(uv_getaddrinfo_t *, int, struct addrinfo *);
typedef void (*uv_work_cb)       (uv_work_t *);
typedef void (*uv_after_work_cb) (uv_work_t *, int);
typedef void (*uv_fs_cb)         (uv_fs_t *);

/* -------------------------------------------------------------------------
 * uv_buf_t
 * ---------------------------------------------------------------------- */
typedef struct uv_buf_t {
    char  *base;
    size_t len;
} uv_buf_t;

static inline uv_buf_t uv_buf_init(char *base, unsigned int len)
{
    uv_buf_t buf;
    buf.base = base;
    buf.len  = (size_t)len;
    return buf;
}

/* -------------------------------------------------------------------------
 * Mutex (single-threaded stub for initial WASM port)
 * ---------------------------------------------------------------------- */
typedef struct { int _dummy; } uv_mutex_t;

static inline int  uv_mutex_init(uv_mutex_t *m)    { (void)m; return 0; }
static inline void uv_mutex_lock(uv_mutex_t *m)    { (void)m; }
static inline void uv_mutex_unlock(uv_mutex_t *m)  { (void)m; }
static inline void uv_mutex_destroy(uv_mutex_t *m) { (void)m; }

/* -------------------------------------------------------------------------
 * Loop
 * ---------------------------------------------------------------------- */
typedef enum {
    UV_RUN_DEFAULT = 0,
    UV_RUN_ONCE,
    UV_RUN_NOWAIT
} uv_run_mode;

struct uv_loop_s {
    void *data;
};

uv_loop_t *uv_default_loop(void);
int        uv_run(uv_loop_t *loop, uv_run_mode mode);
void       uv_loop_close(uv_loop_t *loop);
void       uv_stop(uv_loop_t *loop);

/* -------------------------------------------------------------------------
 * Handle structs
 * ---------------------------------------------------------------------- */

struct uv_handle_s {
    UV_HANDLE_FIELDS
};

/* Stream adds nothing here; concrete stream types add their own fields. */
struct uv_stream_s {
    UV_HANDLE_FIELDS
};

struct uv_tcp_s {
    UV_HANDLE_FIELDS
};

struct uv_tty_s {
    UV_HANDLE_FIELDS
};

struct uv_timer_s {
    UV_HANDLE_FIELDS
    uv_timer_cb  timer_cb;
    uint64_t     repeat;
    uint64_t     timeout;
    int          active;
};

struct uv_signal_s {
    UV_HANDLE_FIELDS
};

struct uv_async_s {
    UV_HANDLE_FIELDS
    uv_async_cb async_cb;
};

struct uv_poll_s {
    UV_HANDLE_FIELDS
    uv_poll_cb poll_cb;
};

struct uv_fs_event_s {
    UV_HANDLE_FIELDS
    uv_fs_event_cb fs_event_cb;
    char path[256];
};

/* -------------------------------------------------------------------------
 * Request structs
 * ---------------------------------------------------------------------- */

struct uv_write_s {
    void        *data;
    uv_write_cb  cb;
};

struct uv_connect_s {
    void          *data;
    uv_connect_cb  cb;
    uv_stream_t   *handle;
};

struct uv_shutdown_s {
    void           *data;
    uv_shutdown_cb  cb;
    uv_stream_t    *handle;
};

struct uv_getaddrinfo_s {
    void               *data;
    uv_getaddrinfo_cb   getaddrinfo_cb;
    struct addrinfo    *addrinfo;
};

struct uv_work_s {
    void            *data;
    uv_work_cb       work_cb;
    uv_after_work_cb after_work_cb;
};

struct uv_fs_s {
    void     *data;
    uv_fs_cb  cb;
    ssize_t   result;
    void     *ptr;
    char      path[256];
};

/* -------------------------------------------------------------------------
 * Handle lifecycle
 * ---------------------------------------------------------------------- */
int  uv_is_closing(const uv_handle_t *h);
void uv_close(uv_handle_t *h, uv_close_cb cb);
int  uv_is_readable(const uv_stream_t *s);
int  uv_is_writable(const uv_stream_t *s);

/* -------------------------------------------------------------------------
 * Timer
 * ---------------------------------------------------------------------- */
int      uv_timer_init(uv_loop_t *loop, uv_timer_t *h);
int      uv_timer_start(uv_timer_t *h, uv_timer_cb cb, uint64_t timeout, uint64_t repeat);
int      uv_timer_stop(uv_timer_t *h);
uint64_t uv_timer_get_repeat(const uv_timer_t *h);
void     uv_timer_set_repeat(uv_timer_t *h, uint64_t repeat);

/* -------------------------------------------------------------------------
 * Signal (no-ops)
 * ---------------------------------------------------------------------- */
static inline int uv_signal_init(uv_loop_t *l, uv_signal_t *h) { (void)l; memset(h, 0, sizeof(*h)); return 0; }
static inline int uv_signal_start(uv_signal_t *h, uv_signal_cb cb, int sig) { (void)h;(void)cb;(void)sig; return 0; }
static inline int uv_signal_stop(uv_signal_t *h) { (void)h; return 0; }

/* -------------------------------------------------------------------------
 * Async
 * ---------------------------------------------------------------------- */
int uv_async_init(uv_loop_t *loop, uv_async_t *h, uv_async_cb cb);
int uv_async_send(uv_async_t *h);

/* -------------------------------------------------------------------------
 * Poll (no-ops; used by the Linux eventfd async path, not reachable in WASM)
 * ---------------------------------------------------------------------- */
static inline int uv_poll_init(uv_loop_t *l, uv_poll_t *h, int fd)   { (void)l;(void)h;(void)fd; return 0; }
static inline int uv_poll_start(uv_poll_t *h, int ev, uv_poll_cb cb) { (void)h;(void)ev;(void)cb; return 0; }
static inline int uv_poll_stop(uv_poll_t *h)                          { (void)h; return 0; }

/* -------------------------------------------------------------------------
 * TCP (no-ops; stratum pool network is not available in browser)
 * ---------------------------------------------------------------------- */
static inline int uv_tcp_init(uv_loop_t *l, uv_tcp_t *h)                           { (void)l; memset(h,0,sizeof(*h)); return 0; }
static inline int uv_tcp_init_ex(uv_loop_t *l, uv_tcp_t *h, unsigned int f)        { (void)l;(void)f; memset(h,0,sizeof(*h)); return 0; }
static inline int uv_tcp_nodelay(uv_tcp_t *h, int en)                              { (void)h;(void)en; return 0; }
static inline int uv_tcp_keepalive(uv_tcp_t *h, int en, unsigned int d)            { (void)h;(void)en;(void)d; return 0; }
static inline int uv_tcp_connect(uv_connect_t *r, uv_tcp_t *h, const struct sockaddr *a, uv_connect_cb cb) { (void)r;(void)h;(void)a;(void)cb; return UV_ENOTSUP; }
static inline int uv_ip4_addr(const char *ip, int port, struct sockaddr_in *addr)  { (void)ip;(void)port;(void)addr; return 0; }
static inline int uv_ip6_addr(const char *ip, int port, struct sockaddr_in6 *addr) { (void)ip;(void)port;(void)addr; return 0; }
static inline int uv_ip4_name(const struct sockaddr_in *src, char *dst, size_t size) {
    if (src == NULL || dst == NULL) return -1;
    if (inet_ntop(AF_INET, &src->sin_addr, dst, size) == NULL) return -1;
    return 0;
}
static inline int uv_ip6_name(const struct sockaddr_in6 *src, char *dst, size_t size) {
    if (src == NULL || dst == NULL) return -1;
    if (inet_ntop(AF_INET6, &src->sin6_addr, dst, size) == NULL) return -1;
    return 0;
}

/* -------------------------------------------------------------------------
 * TTY (no-ops; terminal I/O not available in browser)
 * ---------------------------------------------------------------------- */
typedef enum { UV_TTY_MODE_NORMAL = 0, UV_TTY_MODE_RAW, UV_TTY_MODE_IO } uv_tty_mode_t;

static inline int  uv_tty_init(uv_loop_t *l, uv_tty_t *h, int fd, int r) { (void)l;(void)h;(void)fd;(void)r; memset(h,0,sizeof(*h)); return 0; }
static inline int  uv_tty_set_mode(uv_tty_t *h, uv_tty_mode_t m)         { (void)h;(void)m; return 0; }
static inline int  uv_tty_reset_mode(void)                                { return 0; }
static inline uv_handle_type uv_guess_handle(int fd) { (void)fd; return UV_UNKNOWN_HANDLE; }

/* -------------------------------------------------------------------------
 * Stream read/write (no-ops)
 * ---------------------------------------------------------------------- */
static inline int uv_read_start(uv_stream_t *s, uv_alloc_cb a, uv_read_cb r)  { (void)s;(void)a;(void)r; return 0; }
static inline int uv_read_stop(uv_stream_t *s)                                 { (void)s; return 0; }
static inline int uv_write(uv_write_t *req, uv_stream_t *s, const uv_buf_t *b, unsigned int n, uv_write_cb cb) { (void)req;(void)s;(void)b;(void)n;(void)cb; return UV_ENOTSUP; }
static inline int uv_try_write(uv_stream_t *s, const uv_buf_t *b, unsigned int n) { (void)s; (void)b; (void)n; return UV_ENOTSUP; }
static inline int uv_shutdown(uv_shutdown_t *req, uv_stream_t *s, uv_shutdown_cb cb) { (void)req;(void)s;(void)cb; return 0; }

/* -------------------------------------------------------------------------
 * Filesystem event (no-ops; no filesystem in browser)
 * ---------------------------------------------------------------------- */
static inline int uv_fs_event_init(uv_loop_t *l, uv_fs_event_t *h)             { (void)l; memset(h,0,sizeof(*h)); return 0; }
static inline int uv_fs_event_start(uv_fs_event_t *h, uv_fs_event_cb c, const char *p, unsigned int f) { (void)h;(void)c;(void)p;(void)f; return 0; }
static inline int uv_fs_event_stop(uv_fs_event_t *h)                           { (void)h; return 0; }

/* -------------------------------------------------------------------------
 * DNS (no-ops; use browser's DNS implicitly via WebSocket)
 * ---------------------------------------------------------------------- */
static inline int  uv_getaddrinfo(uv_loop_t *l, uv_getaddrinfo_t *r, uv_getaddrinfo_cb cb, const char *node, const char *svc, const struct addrinfo *hints) {
    (void)l;(void)r;(void)cb;(void)node;(void)svc;(void)hints;
    return UV_ENOTSUP;
}
static inline void uv_freeaddrinfo(struct addrinfo *ai) { (void)ai; }

/* -------------------------------------------------------------------------
 * Work queue (runs synchronously in single-threaded WASM)
 * ---------------------------------------------------------------------- */
int uv_queue_work(uv_loop_t *loop, uv_work_t *req, uv_work_cb work_cb, uv_after_work_cb after_cb);
int uv_cancel(uv_work_t *req);

/* -------------------------------------------------------------------------
 * Filesystem operations (no-ops; use IndexedDB or skip logging to file)
 * ---------------------------------------------------------------------- */
static inline int uv_fs_open(uv_loop_t *l, uv_fs_t *r, const char *p, int f, int m, uv_fs_cb cb) { (void)l;(void)p;(void)f;(void)m;(void)cb; r->result = -1; return -1; }
static inline int uv_fs_close(uv_loop_t *l, uv_fs_t *r, int fd, uv_fs_cb cb)                      { (void)l;(void)r;(void)fd;(void)cb; return 0; }
static inline int uv_fs_read(uv_loop_t *l, uv_fs_t *r, int fd, const uv_buf_t *b, unsigned n, int64_t o, uv_fs_cb cb)  { (void)l;(void)r;(void)fd;(void)b;(void)n;(void)o;(void)cb; return 0; }
static inline int uv_fs_write(uv_loop_t *l, uv_fs_t *r, int fd, const uv_buf_t *b, unsigned n, int64_t o, uv_fs_cb cb) { (void)l;(void)r;(void)fd;(void)b;(void)n;(void)o;(void)cb; return 0; }
static inline int uv_fs_stat(uv_loop_t *l, uv_fs_t *r, const char *p, uv_fs_cb cb)                { (void)l;(void)r;(void)p;(void)cb; return 0; }
static inline void uv_fs_req_cleanup(uv_fs_t *r)                                                   { (void)r; }
static inline void uv_setup_args(int argc, char **argv)                                            { (void)argc; (void)argv; }

/* -------------------------------------------------------------------------
 * OS helpers
 * ---------------------------------------------------------------------- */
uint64_t uv_get_free_memory(void);
uint64_t uv_get_total_memory(void);
int      uv_os_getpid(void);
int      uv_os_getppid(void);
int      uv_os_homedir(char *buf, size_t *size);
int      uv_os_tmpdir(char *buf, size_t *size);
int      uv_exepath(char *buf, size_t *size);
int      uv_cwd(char *buf, size_t *size);
int      uv_chdir(const char *dir);
int      uv_getenv(const char *name, char *buf, size_t *size);
int      uv_os_getenv(const char *name, char *buf, size_t *size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* UV_H */
