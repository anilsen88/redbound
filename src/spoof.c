#include "spoof.h"
#include "mem.h"
#include "vmi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    uint32_t version_code;
    addr_t text_start;
    addr_t text_end;
    addr_t kaslr_mask;
    addr_t alignment;
    addr_t max_offset;
    addr_t special_offset;
} kernel_version_map_t;

static const kernel_version_map_t version_map[] = {
    {0x050F00, 0xffffffff81000000, 0xffffffffa2000000, 0x3fffffff, 0x200000, 0x40000000, 0x1000000},
    {0x051000, 0xffffffff81000000, 0xffffffffa4000000, 0x3fffffff, 0x200000, 0x40000000, 0x1000000},
    {0x051500, 0xffffffff81000000, 0xffffffffa8000000, 0x3fffffff, 0x200000, 0x40000000, 0x1000000},
    {0x051600, 0xffffffff81000000, 0xffffffffa8000000, 0x7fffffff, 0x200000, 0x80000000, 0x1000000},
    {0x060000, 0xffffffff81000000, 0xffffffffc0000000, 0x7fffffff, 0x200000, 0x80000000, 0x2000000},
    {0x060100, 0xffffffff81000000, 0xffffffffc0000000, 0x7fffffff, 0x200000, 0x80000000, 0x2000000}
};

static addr_t original_offset = 0;
static addr_t current_offset = 0;
static bool spoofer_active = false;
static const kernel_version_map_t *current_version_map = NULL;

static const kernel_version_map_t *get_version_map(uint32_t version_code) {
    for (size_t i = 0; i < sizeof(version_map) / sizeof(version_map[0]); i++) {
        if (version_code >= version_map[i].version_code && 
            (i == sizeof(version_map) / sizeof(version_map[0]) - 1 || 
             version_code < version_map[i + 1].version_code)) {
            return &version_map[i];
        }
    }
    return &version_map[0];
}

static uint32_t parse_version_string(const char *version) {
    uint32_t major = 0, minor = 0, patch = 0;
    sscanf(version, "%d.%d.%d", &major, &minor, &patch);
    return (major << 16) | (minor << 8) | patch;
}

bool init_spoofer(vmi_instance_t vmi, spoof_config_t *config) {
    if (!config) return false;
    
    const char *kernel_version = vmi_get_last_error();
    uint32_t version_code = parse_version_string(kernel_version + 12);
    current_version_map = get_version_map(version_code);
    
    srand(time(NULL));
    original_offset = get_kaslr_offset(vmi);
    if (!original_offset) return false;
    
    current_offset = original_offset;
    spoofer_active = true;
    return true;
}

bool spoof_kaslr_offset(vmi_instance_t vmi, addr_t new_offset) {
    if (!spoofer_active || !current_version_map) return false;
    if (!validate_kaslr_offset(new_offset)) return false;

    addr_t kernel_base = get_kernel_base(vmi);
    if (!kernel_base) return false;

    addr_t real_kernel_text = kernel_base + new_offset + current_version_map->special_offset;
    if (!write_memory(vmi, kernel_base + current_version_map->text_start - current_version_map->text_end, 
                     &real_kernel_text, sizeof(real_kernel_text))) {
        return false;
    }

    current_offset = new_offset;
    return true;
}

bool validate_kaslr_offset(addr_t offset) {
    if (!current_version_map) return false;
    if (offset > current_version_map->max_offset) return false;
    if (offset & ~current_version_map->kaslr_mask) return false;
    if (offset & (current_version_map->alignment - 1)) return false;
    return true;
}

bool restore_original_offset(vmi_instance_t vmi) {
    if (!spoofer_active || !original_offset) return false;
    bool result = spoof_kaslr_offset(vmi, original_offset);
    if (result) {
        spoofer_active = false;
    }
    return result;
}

bool is_spoofing_active(void) {
    return spoofer_active;
}

addr_t generate_random_offset(void) {
    if (!current_version_map) return 0;
    addr_t offset;
    do {
        offset = (addr_t)rand() & current_version_map->kaslr_mask;
        offset &= ~(current_version_map->alignment - 1);
    } while (!validate_kaslr_offset(offset));
    return offset;
}

addr_t get_current_spoofed_offset(void) {
    return current_offset;
}
