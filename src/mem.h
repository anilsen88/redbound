#ifndef MEM_H
#define MEM_H

#include <libvmi/libvmi.h>
#include <stdbool.h>
#include <stdint.h>
#include "kernel_config.h"

typedef struct {
    addr_t start;
    addr_t end;
} kernel_region_t;

bool init_memory_subsystem(void);
bool add_kernel_region(addr_t start, addr_t end);
bool read_memory(vmi_instance_t vmi, addr_t address, void *buffer, size_t size);
bool write_memory(vmi_instance_t vmi, addr_t address, void *buffer, size_t size);
bool validate_memory_range(addr_t address, size_t size);
bool is_kernel_address(addr_t address);
addr_t align_address(addr_t address);
bool is_page_aligned(addr_t address);
size_t get_page_size(void);
addr_t get_next_page(addr_t address);
addr_t get_previous_page(addr_t address);
size_t get_offset_in_page(addr_t address);
bool is_same_page(addr_t addr1, addr_t addr2);
size_t get_memory_size(addr_t start, addr_t end);
bool is_address_range_valid(addr_t start, addr_t end);
addr_t get_page_boundary(addr_t address);
bool is_same_page_non_contiguous(addr_t addr1, addr_t addr2);
bool is_address_range_valid_non_contiguous(addr_t start, addr_t end);

#endif // MEM_H
