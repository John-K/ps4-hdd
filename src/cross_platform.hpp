#include <stdint.h>

uint32_t platform_getpagesize();
void platform_release_mmap_region(void *region_start, size_t len);
