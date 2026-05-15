#include "heap.h"
#include "kernel/log.h"
#include "page.h"

// adapted from
// https://git.9front.org/plan9front/plan9front/0861d0d0f283a9917721214fa3dc1c51a778213d/sys/src/9/port/xalloc.c/f.html
// i have no idea what license it is

#undef offsetof
#define offsetof(a, b) ((int)(&(((a *)(0))->b)))
#define nil 0
#define nelem(x) (sizeof(x) / sizeof((x)[0]))

uintptr_t paddr(void *va) {
  if ((uintptr_t)va >= KHEAP_START &&
      (uintptr_t)va < KHEAP_START + KHEAP_INITIAL_SIZE)
    return (uintptr_t)va - KHEAP_START;
  // for identity-mapped early kernel memory
  if ((uintptr_t)va < KHEAP_START)
    return (uintptr_t)va;
  assert(0 && "kernel pannic!");
  return 0;
}

void *kaddr(uintptr_t pa) {
  // pa is an offset within the heap region, convert to virtual address
  if (pa < KHEAP_INITIAL_SIZE)
    return (void *)(pa + KHEAP_START);
  assert(0 && "kernel pannic!");
  return 0;
}

enum {
  nhole = 128,
  magichole = 0x454C4F48, /* HOLE */
};

typedef struct hole_t hole_t;
typedef struct heap_allocator_t heap_allocator_t;
typedef struct block_header_t block_header_t;

struct hole_t {
  uintptr_t addr;
  uintptr_t size;
  uintptr_t top;
  hole_t *link;
};

struct block_header_t {
  uint32_t size;
  uint32_t magix;
  char data[]; // needs to be [] for offsetof
};

struct heap_allocator_t {
  hole_t hole[nhole];
  hole_t *flist;
  hole_t *table;
};

static heap_allocator_t xlists;
void *kheap = NULL;

void xinit(void) {
  hole_t *h, *eh;

  eh = &xlists.hole[nhole - 1];
  for (h = xlists.hole; h < eh; h++)
    h->link = h + 1;

  h->link = NULL; // ensure the last hole has NULL link
  xlists.flist = xlists.hole;

  // set up the initial memory hole that represents our available heap
  h = xlists.flist;
  if (h == NULL) {
    return;
  }

  xlists.flist = h->link;

  h->addr = paddr((void *)KHEAP_START);
  h->size = KHEAP_INITIAL_SIZE;
  h->top = h->addr + h->size;
  h->link = NULL;

  // set this as our only table
  xlists.table = h;

  KLOG(LOG_MODULE_HEAP, "initial heap memory: paddr=%x vaddr=%x size=%x top=%x",
       h->addr, KHEAP_START, h->size, h->top);
  kheap = (void *)1;
}

void *xspanalloc(uint32_t size, int align, uint32_t span) {
  uintptr_t a, v, t;

  a = (uintptr_t)xalloc(size + align + span);
  if (a == 0) {
  }
  KLOG(LOG_MODULE_HEAP, "xspanalloc: %xd %d %xx", size, align, span);

  if (span > 2) {
    v = (a + span) & ~((uintptr_t)span - 1);
    t = v - a;
    if (t > 0)
      xhole(paddr((void *)a), t);
    t = a + span - v;
    if (t > 0)
      xhole(paddr((void *)(v + size + align)), t);
  } else
    v = a;

  if (align > 1)
    v = (v + align) & ~((uintptr_t)align - 1);

  return (void *)v;
}

void *xallocz(uint32_t size, int zero) {
  block_header_t *p;
  hole_t *h, **l;

  /* add room for magix & size overhead, round up to nearest vlong */
  size += BY2V + offsetof(block_header_t, data[0]);
  size &= ~(BY2V - 1);

  cli();
  l = &xlists.table;
  for (h = *l; h; h = h->link) {
    if (h->size >= size) {
      p = (block_header_t *)kaddr(h->addr);
      h->addr += size;
      h->size -= size;
      if (h->size == 0) {
        *l = h->link;
        h->link = xlists.flist;
        xlists.flist = h;
      }
      sti();
      if (zero)
        memset(p, 0, size);
      p->magix = magichole;
      p->size = size;
      KLOG(LOG_MODULE_HEAP, "xalloc %d bytes -> %x", size, p->data);
      return p->data;
    }
    l = &h->link;
  }
  sti();
  assert(0 && "out of memory");
  return nil;
}

void *xalloc(uint32_t size) { return xallocz(size, 1); }

void xfree(void *p) {
  cli();
  block_header_t *x;

  if (p == NULL) {
    sti();
    return;
  }

  x = (block_header_t *)((uintptr_t)p - offsetof(block_header_t, data[0]));

  // validate that x is within heap bounds
  if ((uintptr_t)x < KHEAP_START ||
      (uintptr_t)x >= KHEAP_START + KHEAP_INITIAL_SIZE) {
    KLOG(LOG_MODULE_HEAP, "xfree: block header %x outside heap bounds [%x-%x]",
         x, KHEAP_START, KHEAP_START + KHEAP_INITIAL_SIZE);
    sti();
    return;
  }

  if (x->magix != magichole) {
    // this might be an aligned allocation from xspanalloc
    // aligned allocations should not be freed via kfree
    KLOG(LOG_MODULE_HEAP,
         "xfree: invalid block header at %x (data=%x), possibly "
         "aligned allocation",
         x, p);
    KLOG(LOG_MODULE_HEAP,
         "  aligned allocations from kmalloc_a cannot be freed");
    sti();
    return;
  }

  xhole(paddr(x), x->size);
  KLOG(LOG_MODULE_HEAP, "freed %x (block at %x, size %d)", p, x, x->size);
  sti();
}

void xhole(uintptr_t addr, uintptr_t size) {
  hole_t *h, *c, **l;
  uintptr_t top;

  if (size == 0)
    return;

  top = addr + size;
  cli();
  l = &xlists.table;
  for (h = *l; h; h = h->link) {
    if (h->top == addr) {
      h->size += size;
      h->top = h->addr + h->size;
      c = h->link;
      if (c && h->top == c->addr) {
        h->top += c->size;
        h->size += c->size;
        h->link = c->link;
        c->link = xlists.flist;
        xlists.flist = c;
      }
      sti();
      return;
    }
    if (h->addr > addr)
      break;
    l = &h->link;
  }
  if (h && top == h->addr) {
    h->addr -= size;
    h->size += size;
    sti();
    return;
  }

  if (xlists.flist == nil) {
    sti();
    KLOG(LOG_MODULE_HEAP, "xfree: no free holes, leaked %d bytes", size);
    return;
  }

  h = xlists.flist;
  xlists.flist = h->link;
  h->addr = addr;
  h->top = top;
  h->size = size;
  h->link = *l;
  *l = h;
  sti();
}

void xsummary(void) {
  int i;
  hole_t *h;
  uintptr_t s;

  i = 0;
  for (h = xlists.flist; h; h = h->link)
    i++;
  KLOG(LOG_MODULE_HEAP, "xsummary: %d holes free", i);

  s = 0;
  for (h = xlists.table; h; h = h->link) {
    KLOG(LOG_MODULE_HEAP, "  hole: addr=%x top=%x size=%d", h->addr, h->top,
         h->size);
    s += h->size;
  }
  KLOG(LOG_MODULE_HEAP, "  total free: %d bytes", s);
}

int placement_address = PLACEMENT_ADDRESS;

void *kmalloc_int(uint32_t size, int align, uint32_t *phys) {
  KLOG(LOG_MODULE_HEAP, "somebody wants %d bytes aligned: %d w/ phys %x", size,
       align, phys);
  uint32_t addr = 0;
  if (kheap == NULL) {
    // early allocations before heap is initialized
    // use placement allocator instead
    addr = placement_address;

    if (align && (addr & 0xFFF)) {
      // align to page boundary (4KB)
      addr = (addr + PAGE_SIZE) & ~0xFFF;
    }

    if (phys) {
      *phys = addr; // physical address = virtual before paging
    }

    placement_address = addr + size;
  } else {
    if (align) {
      addr = (uint32_t)xspanalloc(size, PAGE_SIZE, 0);
    } else {
      addr = (uint32_t)xalloc(size);
    }

    if (phys && addr) {
      *phys = virt2phys((void *)addr);
    }
  }
  KLOG(LOG_MODULE_HEAP, "giving them the address %x", addr);
  return (void *)addr;
}

void kfree(void *addr) {
  if (addr == NULL) {
    KLOG(LOG_MODULE_HEAP, "nothing to free");
    return;
  } else if (kheap == NULL) {
    KLOG(LOG_MODULE_HEAP, "kheap is not active");
    return;
  }
  xfree(addr);
}

void *kmalloc_a(uint32_t size) { return kmalloc_int(size, 1, 0); }

void *kmalloc_p(uint32_t size, uint32_t *phys) {
  return kmalloc_int(size, 0, phys);
}

void *kmalloc_ap(uint32_t size, uint32_t *phys) {
  return kmalloc_int(size, 1, phys);
}

void *kmalloc(uint32_t size) { return kmalloc_int(size, 0, 0); }

void kheap_watchdog(void *data) {
  (void)data;
  KLOG(LOG_MODULE_HEAP, "heap watchdog started");

  while (1) {

    hole_t *h;
    int free_chunks = 0;
    uintptr_t free_memory = 0;

    for (h = xlists.table; h != NULL; h = h->link) {
      free_chunks++;
      free_memory += h->size;

      assert(h->addr + h->size == h->top && "heap: hole bounds inconsistent");

      if (h->link != NULL) {
        assert((uintptr_t)h->link >= KHEAP_START &&
               (uintptr_t)h->link < KHEAP_START + KHEAP_INITIAL_SIZE &&
               "heap: hole link pointer outside heap");
      }
    }

// create a map of free regions to avoid checking them for allocated blocks
#define MAX_FREE_REGIONS 32
    struct {
      uintptr_t start;
      uintptr_t end;
    } free_regions[MAX_FREE_REGIONS];
    int free_region_count = 0;

    for (h = xlists.table; h != NULL && free_region_count < MAX_FREE_REGIONS;
         h = h->link) {
      free_regions[free_region_count].start = KHEAP_START + h->addr;
      free_regions[free_region_count].end = KHEAP_START + h->top;
      free_region_count++;
    }

    uintptr_t current_addr = KHEAP_START;
    int valid_blocks = 0;
    uintptr_t allocated_memory = 0;

    while (current_addr < KHEAP_START + KHEAP_INITIAL_SIZE) {
      // check if current address is in a free region
      bool in_free_region = false;
      for (int i = 0; i < free_region_count; i++) {
        if (current_addr >= free_regions[i].start &&
            current_addr < free_regions[i].end) {
          // skip to the end of this free region
          current_addr = free_regions[i].end;
          in_free_region = true;
          break;
        }
      }

      if (in_free_region) {
        continue;
      }

      block_header_t *block = (block_header_t *)current_addr;

      // Basic validation for what might be a block header
      if (block->size >= sizeof(block_header_t) &&
          block->size < KHEAP_INITIAL_SIZE &&
          (current_addr + block->size) <= (KHEAP_START + KHEAP_INITIAL_SIZE)) {

        if (block->magix == magichole) {
          valid_blocks++;
          allocated_memory += block->size;

          current_addr += block->size;
        } else {
          if ((block->size & 0x7) == 0 && block->size > 0) {
            assert(0 && "heap: corrupted block magic");
          } else {
            current_addr += 8;
          }
        }
      } else {
        current_addr += 8;
      }
    }

    KLOG(LOG_MODULE_HEAP,
         "stats: %d valid (%d bytes), %d free holes (%d bytes)", valid_blocks,
         allocated_memory, free_chunks, free_memory);

    // xsummary();

    for (volatile int i = 0; i < 5000000; i++) {
      asm volatile("nop");
    }
  }
}