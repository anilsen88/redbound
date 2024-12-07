#ifndef KERNEL_CONFIG_H
#define KERNEL_CONFIG_H

#include <stdint.h>

#define KERNEL_MIN_ADDR 0xffff800000000000ULL
#define KERNEL_MAX_ADDR 0xffffffffffffffffULL

void set_kernel_address_ranges(uint64_t *min_addr, uint64_t *max_addr);
bool read_kernel_address_ranges_from_env(uint64_t *min_addr, uint64_t *max_addr);
bool read_kernel_address_ranges_from_system(uint64_t *min_addr, uint64_t *max_addr);

#endif // KERNEL_CONFIG_H 