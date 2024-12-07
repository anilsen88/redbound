#ifndef VMI_H
#define VMI_H

#include <libvmi/libvmi.h>
#include <stdbool.h>

bool init_vmi(vmi_instance_t *vmi, char *name);
void cleanup_vmi(vmi_instance_t *vmi);

addr_t get_kernel_base(vmi_instance_t vmi);
addr_t get_kaslr_offset(vmi_instance_t vmi);

const char* vmi_get_last_error(void);

#endif // VMI_H
