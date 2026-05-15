#pragma once
#include "drivers/serial.h"
#include "libc/string.h"

typedef enum {
  LOG_MODULE_UDP = 0,
  LOG_MODULE_IP,
  LOG_MODULE_ARP,
  LOG_MODULE_DHCP,
  LOG_MODULE_NET,
  LOG_MODULE_9P,
  LOG_MODULE_VFS,
  LOG_MODULE_FAT16,
  LOG_MODULE_RTL8139,
  LOG_MODULE_KERNEL,
  LOG_MODULE_TASK,
  LOG_MODULE_MEM,
  LOG_MODULE_PCI,
  LOG_MODULE_COUNT
} log_module_t;

extern int log_enabled[LOG_MODULE_COUNT];

static inline const char *log_module_name(log_module_t m) {
  switch (m) {
  case LOG_MODULE_UDP:
    return "udp";
  case LOG_MODULE_IP:
    return "ip";
  case LOG_MODULE_ARP:
    return "arp";
  case LOG_MODULE_DHCP:
    return "dhcp";
  case LOG_MODULE_NET:
    return "net";
  case LOG_MODULE_9P:
    return "9p";
  case LOG_MODULE_VFS:
    return "vfs";
  case LOG_MODULE_FAT16:
    return "fat16";
  case LOG_MODULE_RTL8139:
    return "rtl8139";
  case LOG_MODULE_KERNEL:
    return "kernel";
  case LOG_MODULE_TASK:
    return "task";
  case LOG_MODULE_MEM:
    return "mem";
  case LOG_MODULE_PCI:
    return "pci";
  default:
    return "unknown";
  }
}

static inline log_module_t log_module_from_name(const char *name) {
  for (int i = 0; i < LOG_MODULE_COUNT; i++) {
    if (strcmp(name, log_module_name((log_module_t)i)) == 0)
      return (log_module_t)i;
  }
  return LOG_MODULE_COUNT;
}

#define KLOG(module, ...)                                                      \
  do {                                                                         \
    if (log_enabled[module])                                                   \
      serial_printf(tick, __FILE__, __LINE__, __VA_ARGS__);                    \
  } while (0)
