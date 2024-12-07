#include "mem.h"
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_KERNEL_REGIONS 16

static size_t system_page_size = 0;
static kernel_region_t kernel_regions[MAX_KERNEL_REGIONS];
static size_t num_kernel_regions = 0;

bool safe_add(addr_t a, addr_t b, addr_t *result) {
    if (__builtin_add_overflow(a, b, result)) {
        return false;
    }
    return true;
}

bool init_memory_subsystem(void) {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) {
        return false;
    }
    system_page_size = (size_t)page_size;
    num_kernel_regions = 0;

    uint64_t min_addr, max_addr;
    set_kernel_address_ranges(&min_addr, &max_addr);

    add_kernel_region(min_addr, max_addr);
    return true;
}

bool add_kernel_region(addr_t start, addr_t end) {
    if (num_kernel_regions >= MAX_KERNEL_REGIONS || start > end) {
        return false;
    }
    
    kernel_regions[num_kernel_regions].start = start;
    kernel_regions[num_kernel_regions].end = end;
    num_kernel_regions++;
    return true;
}

bool is_kernel_address(addr_t address) {
    if (!is_page_aligned(address)) {
        return false;
    }
    for (size_t i = 0; i < num_kernel_regions; i++) {
        if (address >= kernel_regions[i].start && address <= kernel_regions[i].end) {
            return true;
        }
    }
    return false;
}

bool validate_memory_range(addr_t address, size_t size) {
    if (!size) {
        return false;
    }

    addr_t end_address;
    if (!safe_add(address, size - 1, &end_address)) {
        return false;
    }

    bool start_valid = false;
    bool end_valid = false;
    
    for (size_t i = 0; i < num_kernel_regions; i++) {
        if (address >= kernel_regions[i].start && address <= kernel_regions[i].end) {
            start_valid = true;
        }
        if (end_address >= kernel_regions[i].start && end_address <= kernel_regions[i].end) {
            end_valid = true;
        }
        if (start_valid && end_valid) {
            return true;
        }
    }
    
    return false;
}

bool is_same_page_non_contiguous(addr_t addr1, addr_t addr2) {
    return (align_address(addr1) == align_address(addr2)) || 
           (is_kernel_address(addr1) && is_kernel_address(addr2));
}

bool is_address_range_valid_non_contiguous(addr_t start, addr_t end) {
    return validate_memory_range(start, get_memory_size(start, end));
}

bool read_memory(vmi_instance_t vmi, addr_t address, void *buffer, size_t size) {
    if (!buffer || !validate_memory_range(address, size)) {
        return false;
    }
    return vmi_read_va(vmi, address, 0, buffer, size) == VMI_SUCCESS;
}

bool write_memory(vmi_instance_t vmi, addr_t address, void *buffer, size_t size) {
    if (!buffer || !validate_memory_range(address, size)) {
        return false;
    }
    return vmi_write_va(vmi, address, 0, buffer, size) == VMI_SUCCESS;
}

addr_t align_address(addr_t address) {
    return address & ~(system_page_size - 1);
}

bool is_page_aligned(addr_t address) {
    return (address & (system_page_size - 1)) == 0;
}

size_t get_page_size(void) {
    return system_page_size;
}

addr_t get_next_page(addr_t address) {
    addr_t next_page;
    if (!safe_add(align_address(address), system_page_size, &next_page)) {
        return 0; // Indicate an error or overflow
    }
    return next_page;
}

addr_t get_previous_page(addr_t address) {
    addr_t previous_page;
    if (!safe_add(align_address(address), -system_page_size, &previous_page)) {
        return 0; // Indicate an error or overflow
    }
    return previous_page;
}

size_t get_offset_in_page(addr_t address) {
    return address & (system_page_size - 1);
}

bool is_same_page(addr_t addr1, addr_t addr2) {
    return is_same_page_non_contiguous(addr1, addr2);
}

size_t get_memory_size(addr_t start, addr_t end) {
    addr_t size;
    if (!safe_add(end, 1 - start, &size)) {
        return 0; // Indicate an error or overflow
    }
    return size;
}

bool is_address_range_valid(addr_t start, addr_t end) {
    return is_address_range_valid_non_contiguous(start, end);
}

addr_t get_page_boundary(addr_t address) {
    return align_address(address + system_page_size - 1);
}

int mask_current_version(unsigned int offset) {
    return offset & CURRENT_VERSION_MASK
}

int is_within_range(unsigned int offset, unsigned int min, unsigned int max) {
    return (offset >= min && offset <= max);
}
