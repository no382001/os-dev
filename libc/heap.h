#pragma once
#include "libc/types.h"

#include "heap.h"
#include "page.h"

#define nil 0
#define nelem(x) (sizeof(x) / sizeof((x)[0]))

#define BY2PG PAGE_SIZE // bytes per page
#define BY2V 2          // boundary align

void xinit(void);
void *xspanalloc(uint16_t size, int align, uint16_t span);
void *xallocz(uint16_t size, int zero);
void *xalloc(uint16_t size);
void xfree(void *p);
int xmerge(void *vp, void *vq);
void xhole(uintptr_t addr, uintptr_t size);
void xsummary(void);

uint32_t kmalloc_int(uint32_t size, int align, uint32_t *phys);
uint32_t kmalloc_a(uint32_t size);
uint32_t kmalloc_p(uint32_t size, uint32_t *phys);
uint32_t kmalloc_ap(uint32_t size, uint32_t *phys);
uint32_t kmalloc(uint32_t size);
void kfree(void *addr);