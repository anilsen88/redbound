#include "mem.h"
#include <string.h>

#define PAGE_SIZE 4096
#define KERNEL_MIN_ADDR 0xffff800000000000ULL
#define KERNEL_MAX_ADDR 0xffffffffffffffffULL

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

bool validate_memory_range(addr_t address, size_t size) {
    if (!size || address + size < address || !is_kernel_address(address) || !is_kernel_address(address + size - 1)) {
        return false;
    }
    return true;
}

bool is_kernel_address(addr_t address) {
    return (address >= KERNEL_MIN_ADDR && address <= KERNEL_MAX_ADDR);
}

addr_t align_address(addr_t address) {
    return address & ~(PAGE_SIZE - 1);
}

bool is_page_aligned(addr_t address) {
    return (address & (PAGE_SIZE - 1)) == 0;
}

size_t get_page_size(void) {
    return PAGE_SIZE;
}

addr_t get_next_page(addr_t address) {
    return align_address(address) + PAGE_SIZE;
}

addr_t get_previous_page(addr_t address) {
    return align_address(address) - PAGE_SIZE;
}

size_t get_offset_in_page(addr_t address) {
    return address & (PAGE_SIZE - 1);
}

bool is_same_page(addr_t addr1, addr_t addr2) {
    return align_address(addr1) == align_address(addr2);
}

size_t get_memory_size(addr_t start, addr_t end) {
    return end - start + 1;
}

bool is_address_range_valid(addr_t start, addr_t end) {
    return validate_memory_range(start, get_memory_size(start, end));
}

addr_t get_page_boundary(addr_t address) {
    return align_address(address + PAGE_SIZE - 1);
}
