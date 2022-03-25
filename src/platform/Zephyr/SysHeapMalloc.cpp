/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "SysHeapMalloc.h"

#include <lib/support/CodeUtils.h>
#include <system/SystemError.h>

extern "C" {
#include <init.h>
#include <sys/mutex.h>
#include <sys/sys_heap.h>
}

#include <cstdint>
#include <cstring>

namespace {

constexpr size_t kMallocAlignment = sizeof(void *);

uint8_t sHeapMemory[8192] alignas(8);
sys_heap sHeap;
SYS_MUTEX_DEFINE(sLock);

static int heapInit(const struct device * unused)
{
    sys_heap_init(&sHeap, sHeapMemory, sizeof(sHeapMemory));
    return 0;
}

} // namespace

SYS_INIT(heapInit, POST_KERNEL, 0);

extern "C" {

void * malloc(size_t size)
{
    return sys_heap_aligned_alloc(&sHeap, kMallocAlignment, size);
}

void * calloc(size_t num, size_t size)
{
    void * mem = malloc(num * size);

    if (mem)
    {
        memset(mem, 0, num * size);
    }

    return mem;
}

void * realloc(void * mem, size_t size)
{
    return sys_heap_aligned_realloc(&sHeap, mem, kMallocAlignment, size);
}

void free(void * mem)
{
    sys_heap_free(&sHeap, mem);
}

/*
void * _malloc_r(_reent *, size_t size)
{
    return malloc(size);
}

void * _calloc_r(_reent *, size_t num, size_t size)
{
    return calloc(num, size);
}

void * _realloc_r(_reent *, void * mem, size_t size)
{
    return realloc(mem, size);
}

void _free_r(_reent *, void * mem)
{
    free(mem);
}
*/
}

namespace chip {
namespace DeviceLayer {
namespace Heap {

CHIP_ERROR GetStats(Stats & stats)
{
    sys_heap_runtime_stats sysHeapStats;
    ReturnErrorOnFailure(System::MapErrorZephyr(sys_heap_runtime_stats_get(&sHeap, &sysHeapStats)));

    stats.free    = sysHeapStats.free_bytes;
    stats.used    = sysHeapStats.allocated_bytes;
    stats.maxUsed = sysHeapStats.max_allocated_bytes;

    return CHIP_NO_ERROR;
}

CHIP_ERROR ResetHeapHighWatermark()
{
    return System::MapErrorZephyr(sys_heap_runtime_stats_reset(&sHeap));
}

} // namespace Heap
} // namespace DeviceLayer
} // namespace chip