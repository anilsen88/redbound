#include "kernel_config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

void set_kernel_address_ranges(uint64_t *min_addr, uint64_t *max_addr) {
    if (!read_kernel_address_ranges_from_env(min_addr, max_addr) &&
        !read_kernel_address_ranges_from_system(min_addr, max_addr)) {

        if (min_addr) {
            *min_addr = KERNEL_MIN_ADDR;
        }
        if (max_addr) {
            *max_addr = KERNEL_MAX_ADDR;
        }
    }
}

bool read_kernel_address_ranges_from_env(uint64_t *min_addr, uint64_t *max_addr) {
    char *min_addr_str = getenv("KERNEL_MIN_ADDR");
    char *max_addr_str = getenv("KERNEL_MAX_ADDR");

    if (min_addr_str && max_addr_str) {
        char *endptr;
        errno = 0; 
        *min_addr = strtoull(min_addr_str, &endptr, 16);
        if (errno != 0 || *endptr != '\0') {
            fprintf(stderr, "Invalid KERNEL_MIN_ADDR: %s\n", min_addr_str);
            return false;
        }
        errno = 0; 
        *max_addr = strtoull(max_addr_str, &endptr, 16);
        if (errno != 0 || *endptr != '\0') {
            fprintf(stderr, "Invalid KERNEL_MAX_ADDR: %s\n", max_addr_str);
            return false;
        }
        return true;
    }
    return false;
}

bool read_kernel_address_ranges_from_system(uint64_t *min_addr, uint64_t *max_addr) {
    FILE *fp = fopen("/proc/iomem", "r");
    if (!fp) {
        perror("Failed to open /proc/iomem");
        return false;
    }

    char line[256];
    bool found_kernel_range = false;
    uint64_t local_min = 0, local_max = 0;

    while (fgets(line, sizeof(line), fp)) {
        uint64_t start, end;
        char name[64];

        if (sscanf(line, "%lx-%lx : %63s", &start, &end, name) == 3) {
            if (strstr(name, "kernel") || strstr(name, "System RAM")) {
                if (!found_kernel_range) {
                    local_min = start;
                    found_kernel_range = true;
                }
                local_max = end;
            }
        }
    }

    fclose(fp);

    if (found_kernel_range) {
        if (min_addr) *min_addr = local_min;
        if (max_addr) *max_addr = local_max;
        return true;
    }

    return false;
} 