/* XMRig
 * Copyright (c) 2018-2023 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2023 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* Emscripten/WASM platform implementation of VirtualMemory.
 *
 * Replaces mmap/mprotect/mlock with malloc-based equivalents. Memory
 * protection is a no-op since the browser sandbox provides isolation.
 * A fixed-size pool per common block size avoids malloc/free churn for
 * the hot scratchpad and cache allocation paths.
 */

#include "crypto/common/VirtualMemory.h"
#include "crypto/common/portable/mm_malloc.h"

#include <mutex>
#include <vector>
#include <cstddef>
#include <cstdint>


namespace {


struct PoolBucket {
    const size_t blockSize;
    std::vector<uint8_t *> freeList;
    std::mutex mtx;
};

// Pre-defined size classes covering the common scratchpad/cache sizes.
// Blocks are recycled rather than freed to avoid heap fragmentation.
static PoolBucket g_pools[] = {
    { 2ULL  * 1024 * 1024,  {}, {} },   // 2 MB  – CryptoNight / RandomX scratchpad
    { 16ULL * 1024 * 1024,  {}, {} },   // 16 MB – RandomX cache (reduced)
    { 256ULL * 1024 * 1024, {}, {} },   // 256 MB – large dataset blocks
};


static PoolBucket *findBucket(size_t size)
{
    for (auto &b : g_pools) {
        if (b.blockSize == size) {
            return &b;
        }
    }
    return nullptr;
}


static void *poolAlloc(size_t size)
{
    PoolBucket *bucket = findBucket(size);
    if (bucket) {
        std::lock_guard<std::mutex> lock(bucket->mtx);
        if (!bucket->freeList.empty()) {
            uint8_t *ptr = bucket->freeList.back();
            bucket->freeList.pop_back();
            return ptr;
        }
    }
    // 64-byte cache-line alignment, same as the native fallback path.
    return _mm_malloc(size, 64);
}


static void poolFree(void *ptr, size_t size)
{
    if (!ptr) {
        return;
    }
    PoolBucket *bucket = findBucket(size);
    if (bucket) {
        std::lock_guard<std::mutex> lock(bucket->mtx);
        bucket->freeList.push_back(static_cast<uint8_t *>(ptr));
        return;
    }
    _mm_free(ptr);
}


} // anonymous namespace


// ---------------------------------------------------------------------------
// Static platform API
// ---------------------------------------------------------------------------

bool xmrig::VirtualMemory::isHugepagesAvailable()
{
    return false;
}


bool xmrig::VirtualMemory::isOneGbPagesAvailable()
{
    return false;
}


// Memory protection is meaningless inside the WASM sandbox — always succeed.
bool xmrig::VirtualMemory::protectRW(void *, size_t)
{
    return true;
}


bool xmrig::VirtualMemory::protectRWX(void *, size_t)
{
    return true;
}


bool xmrig::VirtualMemory::protectRX(void *, size_t)
{
    return true;
}


// Executable memory for JIT: plain malloc is fine; the WASM sandbox enforces
// security at the engine level, not via page permissions.
void *xmrig::VirtualMemory::allocateExecutableMemory(size_t size, bool)
{
    return poolAlloc(size);
}


void *xmrig::VirtualMemory::allocateLargePagesMemory(size_t size)
{
    return poolAlloc(size);
}


void *xmrig::VirtualMemory::allocateOneGbPagesMemory(size_t)
{
    return nullptr;
}


bool xmrig::VirtualMemory::adviseLargePages(void *, size_t)
{
    return false;
}


// No instruction cache flush needed — WASM JIT handles this internally.
void xmrig::VirtualMemory::flushInstructionCache(void *, size_t)
{
}


void xmrig::VirtualMemory::freeLargePagesMemory(void *p, size_t size)
{
    poolFree(p, size);
}


void xmrig::VirtualMemory::osInit(size_t)
{
}


// ---------------------------------------------------------------------------
// Instance helpers (called from VirtualMemory.cpp constructor/destructor)
// ---------------------------------------------------------------------------

bool xmrig::VirtualMemory::allocateLargePagesMemory()
{
    m_scratchpad = static_cast<uint8_t *>(allocateLargePagesMemory(m_size));
    if (m_scratchpad) {
        // Report as hugepages so the destructor takes the correct free path.
        m_flags.set(FLAG_HUGEPAGES, true);
        return true;
    }
    return false;
}


bool xmrig::VirtualMemory::allocateOneGbPagesMemory()
{
    return false;
}


void xmrig::VirtualMemory::freeLargePagesMemory()
{
    // FLAG_LOCK is never set on WASM (no mlock), so skip the unlock branch.
    freeLargePagesMemory(m_scratchpad, m_size);
}
