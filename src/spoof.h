#ifndef SPOOF_H
#define SPOOF_H

#include <libvmi/libvmi.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    addr_t target_offset;     
    bool randomize;           
    uint32_t interval;
    int debug_level;
    char log_file[256];        
} spoof_config_t;

bool init_spoofer(vmi_instance_t vmi, spoof_config_t *config);
bool spoof_kaslr_offset(vmi_instance_t vmi, addr_t new_offset);
bool restore_original_offset(vmi_instance_t vmi);
bool validate_kaslr_offset(addr_t offset);
bool is_spoofing_active(void);
addr_t generate_random_offset(void);
addr_t get_current_spoofed_offset(void);

#endif // SPOOF_H
