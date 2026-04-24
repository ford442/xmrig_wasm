/* XMRig WASM Thread Stub
 * Replaces std::thread with synchronous execution for Emscripten builds.
 */

#ifndef XMRIG_THREAD_WASM_H
#define XMRIG_THREAD_WASM_H

#include "backend/common/interfaces/IWorker.h"
#include <cstdint>

namespace xmrig {

template<class T>
class Thread
{
public:
    XMRIG_DISABLE_COPY_MOVE_DEFAULT(Thread)

    inline Thread(IBackend *backend, size_t id, const T &config) : m_id(id), m_config(config), m_backend(backend) {}

    inline ~Thread() { delete m_worker; }

    inline void start(void *(*callback)(void *))
    {
        // In WASM without pthreads, run synchronously
        callback(this);
    }

    inline const T &config() const                  { return m_config; }
    inline IBackend *backend() const                { return m_backend; }
    inline IWorker *worker() const                  { return m_worker; }
    inline size_t id() const                        { return m_id; }
    inline void setWorker(IWorker *worker)          { m_worker = worker; }

private:
    const size_t m_id    = 0;
    const T m_config;
    IBackend *m_backend;
    IWorker *m_worker       = nullptr;
};

} // namespace xmrig

#endif
