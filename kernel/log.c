#include "kernel/log.h"

int log_enabled[LOG_MODULE_COUNT] = {
    [LOG_MODULE_UDP] = 0,    [LOG_MODULE_IP] = 0,    [LOG_MODULE_ARP] = 0,
    [LOG_MODULE_DHCP] = 0,   [LOG_MODULE_NET] = 0,   [LOG_MODULE_9P] = 1,
    [LOG_MODULE_VFS] = 0,    [LOG_MODULE_FAT16] = 0, [LOG_MODULE_RTL8139] = 0,
    [LOG_MODULE_KERNEL] = 1, [LOG_MODULE_TASK] = 0,  [LOG_MODULE_MEM] = 0,
    [LOG_MODULE_PCI] = 0,    [LOG_MODULE_HEAP] = 1,
};
