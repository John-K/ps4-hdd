#include <stdint.h>

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

uint32_t platform_getpagesize() {
#if defined(__WIN32) || defined(__WIN64)
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    return sys_info.dwPageSize
#else
    return getpagesize();
#endif
}

void platform_release_mmap_region(void *region_start, size_t len) {
#if defined(__WIN32) || defined(__WIN64)
	VirtualFree(region_start, len, MEM_DECOMMIT);
    VirtualAlloc(region_start, len, MEM_COMMIT, PAGE_NOCACHE);
#else
    madvise(region_start, len, MADV_DONTNEED);
#endif
}
