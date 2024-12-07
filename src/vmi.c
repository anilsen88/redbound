#include "vmi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char error_buffer[256];

bool init_vmi(vmi_instance_t *vmi, char *name) {
    if (!name) {
        snprintf(error_buffer, sizeof(error_buffer), "VM name is NULL");
        return false;
    }

    if (vmi_init(vmi, VMI_XEN, name, VMI_INIT_COMPLETE | VMI_INIT_EVENTS, 
                 NULL, NULL) == VMI_FAILURE) {
        snprintf(error_buffer, sizeof(error_buffer), "Failed to initialize VMI");
        return false;
    }

    return true;
}

void cleanup_vmi(vmi_instance_t *vmi) {
    if (vmi) {
        vmi_destroy(*vmi);
    }
}

addr_t get_kernel_base(vmi_instance_t vmi) {
    addr_t kernel_base = 0, banner_addr = 0, version_addr = 0;
    char kernel_version[512] = {0}, version_string[64] = {0};
    uint32_t major = 0, minor = 0, patch = 0;
    const char *version_symbols[] = {"linux_banner", "linux_proc_banner", "linux_version_code"};
    
    for (size_t i = 0; i < sizeof(version_symbols) / sizeof(version_symbols[0]); i++) {
        if (vmi_translate_ksym2v(vmi, version_symbols[i], &version_addr) == VMI_SUCCESS) {
            if (vmi_read_str_va(vmi, version_addr, 0, kernel_version, sizeof(kernel_version) - 1) > 0) {
                char *ptr = kernel_version;
                while (*ptr) {
                    if (sscanf(ptr, "%d.%d.%d", &major, &minor, &patch) == 3) {
                        snprintf(version_string, sizeof(version_string), "%d.%d.%d", major, minor, patch);
                        break;
                    }
                    ptr++;
                }
                if (major) break;
            }
        }
    }

    addr_t dtb = 0, start_addr = 0xffffffff80000000, end_addr = 0xffffffffa0000000;
    uint8_t signature[] = {0x48, 0x8d, 0x05};
    size_t sig_size = sizeof(signature);
    uint8_t buf[16] = {0};

    if (vmi_get_dtb(vmi, &dtb) == VMI_SUCCESS) {
        for (addr_t addr = start_addr; addr < end_addr; addr += 0x1000) {
            if (vmi_read_va(vmi, addr, 0, buf, sizeof(buf)) == VMI_SUCCESS) {
                int match = 1;
                for (size_t i = 0; i < sig_size && match; i++) {
                    if (buf[i] != signature[i]) match = 0;
                }
                
                if (match) {
                    addr_t potential_base = addr;
                    uint8_t verify_buf[32] = {0};
                    
                    if (vmi_read_va(vmi, potential_base - 0x1000, 0, verify_buf, sizeof(verify_buf)) == VMI_SUCCESS) {
                        if (verify_buf[0] == 0x55 && verify_buf[1] == 0x48) {
                            kernel_base = potential_base;
                            break;
                        }
                    }
                }
            }
        }
    }

    if (!kernel_base) {
        if (vmi_translate_ksym2v(vmi, "_text", &kernel_base) == VMI_SUCCESS) {
            uint32_t version_code = (major << 16) | (minor << 8) | patch;
            snprintf(error_buffer, sizeof(error_buffer), "KB:0x%lx Ver:%s(0x%x)", 
                    kernel_base, version_string, version_code);
            return kernel_base;
        }
        snprintf(error_buffer, sizeof(error_buffer), "KB detection failed");
        return 0;
    }

    uint32_t version_code = (major << 16) | (minor << 8) | patch;
    snprintf(error_buffer, sizeof(error_buffer), "KB:0x%lx Ver:%s(0x%x)", 
            kernel_base, version_string, version_code);
    return kernel_base;
}

addr_t get_kaslr_offset(vmi_instance_t vmi) {
    addr_t kernel_base = get_kernel_base(vmi);
    if (!kernel_base) {
        return 0;
    }

    return kernel_base - 0xffffffff81000000;
}

const char* vmi_get_last_error(void) {
    return error_buffer;
}
