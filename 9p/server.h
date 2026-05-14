#pragma once
#include "libc/types.h"
#include "fs/vfs.h"

void ninep_server_init(vfs *v);
void ninep_server_handle(void *data, uint32_t len, uint8_t *src_ip,
                         uint16_t src_port);
