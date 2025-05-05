#include "libc/heap.h"
#include "libc/page.h"

/** /
#undef serial_debug
#define serial_debug(...)
/**/

// end is defined in the linker script.
// extern uint32_t end;
uint32_t placement_address = PLACEMENT_ADDRESS; //(uint32_t)&end;
extern page_directory_t *kernel_directory;

kernel_heap_bitmap_t *kheap = 0;

void kernel_heap_bitmap_init(kernel_heap_bitmap_t *heap) {
  kheap = (kernel_heap_bitmap_t *)kmalloc(sizeof(kernel_heap_bitmap_t));
  heap->first_block = 0;
}

int kernel_heap_bitmap_add_block(kernel_heap_bitmap_t *heap, uintptr_t addr,
                                 uint32_t size, uint32_t block_size) {
  kernel_heap_bitmap_block_t *block;
  uint32_t block_count;
  uint32_t i;
  uint8_t *bitmap;

  block = (kernel_heap_bitmap_block_t *)addr;
  block->block_size = size - sizeof(kernel_heap_bitmap_block_t);
  block->allocation_unit_size = block_size;

  block->next = heap->first_block;
  heap->first_block = block;

  block_count = block->block_size / block->allocation_unit_size;
  bitmap = (uint8_t *)&block[1];

  /* clear bitmap */
  for (i = 0; i < block_count; ++i) {
    bitmap[i] = 0;
  }

  /* reserve room for bitmap */
  block_count = (block_count / block_size) * block_size < block_count
                    ? block_count / block_size + 1
                    : block_count / block_size;
  for (i = 0; i < block_count; ++i) {
    bitmap[i] = 5;
  }

  block->last_free_block = block_count - 1;
  block->used_blocks = block_count;

  return 1;
}

static uint8_t kernel_heap_bitmap_get_next_id(uint8_t left_id,
                                              uint8_t right_id) {
  uint8_t new_id;
  for (new_id = left_id + 1; new_id == right_id || new_id == 0; ++new_id)
    ;
  return new_id;
}
void *kernel_heap_bitmap_alloc(kernel_heap_bitmap_t *heap, uint32_t size) {
  kernel_heap_bitmap_block_t *block;
  uint8_t *bitmap;
  uint32_t block_count;
  uint32_t i, consecutive_blocks, z;
  uint32_t blocks_needed;
  uint8_t next_id;

  /* iterate through blocks */
  for (block = heap->first_block; block; block = block->next) {
    /* check if block has enough room */
    if (block->block_size -
            (block->used_blocks * block->allocation_unit_size) >=
        size) {

      block_count = block->block_size / block->allocation_unit_size;
      blocks_needed =
          (size / block->allocation_unit_size) * block->allocation_unit_size <
                  size
              ? size / block->allocation_unit_size + 1
              : size / block->allocation_unit_size;
      bitmap = (uint8_t *)&block[1];

      // start search from last free block as an optimization
      uint32_t start_idx = (block->last_free_block + 1 >= block_count)
                               ? 0
                               : block->last_free_block + 1;

      // search the entire bitmap, not just up to last_free_block
      for (uint32_t search_count = 0; search_count < block_count;
           ++search_count) {
        i = (start_idx + search_count) % block_count;

        if (bitmap[i] == 0) {
          /* count consecutive free blocks */
          for (consecutive_blocks = 0; bitmap[i + consecutive_blocks] == 0 &&
                                       consecutive_blocks < blocks_needed &&
                                       (i + consecutive_blocks) < block_count;
               ++consecutive_blocks)
            ;

          /* we have enough consecutive blocks, allocate them */
          if (consecutive_blocks == blocks_needed) {
            /* find ID that does not match left or right neighbors */
            next_id = kernel_heap_bitmap_get_next_id(
                bitmap[i - 1], bitmap[i + consecutive_blocks]);

            /* mark blocks as allocated with the new ID */
            for (z = 0; z < consecutive_blocks; ++z) {
              bitmap[i + z] = next_id;
            }

            /* update last free block for optimization */
            block->last_free_block = (i + blocks_needed) - 2;

            /* update count of used blocks */
            block->used_blocks += consecutive_blocks;

            /* return pointer to the allocated memory */
            return (void *)(i * block->allocation_unit_size +
                            (uintptr_t)&block[1]);
          }

          /* skip ahead */
          search_count += (consecutive_blocks - 1);
          continue;
        }
      }
    }
  }

  /* no memory available */
  return NULL;
}

void kernel_heap_bitmap_free(kernel_heap_bitmap_t *heap, void *ptr) {
  kernel_heap_bitmap_block_t *block;
  uintptr_t ptr_offset;
  uint32_t block_index, i;
  uint8_t *bitmap;
  uint8_t id;
  uint32_t max_blocks;

  /* find which block contains this pointer */
  for (block = heap->first_block; block; block = block->next) {
    if ((uintptr_t)ptr > (uintptr_t)block &&
        (uintptr_t)ptr < (uintptr_t)block + sizeof(kernel_heap_bitmap_block_t) +
                             block->block_size) {
      /* calculate offset from start of data area */
      ptr_offset = (uintptr_t)ptr - (uintptr_t)&block[1];

      /* calculate block index in bitmap */
      block_index = ptr_offset / block->allocation_unit_size;

      /* get bitmap pointer */
      bitmap = (uint8_t *)&block[1];

      /* get allocation ID */
      id = bitmap[block_index];

      /* maximum number of blocks */
      max_blocks = block->block_size / block->allocation_unit_size;

      /* clear all blocks with matching ID */
      for (i = block_index; bitmap[i] == id && i < max_blocks; ++i) {
        bitmap[i] = 0;
      }

      /* update count of used blocks */
      block->used_blocks -= i - block_index;
      return;
    }
  }

  /* error: pointer not found in any block */
  return;
}

uint32_t kmalloc_int(uint32_t size, int align, uint32_t *phys) {
  serial_debug("somebody wants %d bytes aligned: %d w/ phys %x", size, align,
               phys);
  uint32_t addr;
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
      // for aligned allocations, request extra space and do manual alignment
      uint32_t extra = PAGE_SIZE;
      addr = (uint32_t)kernel_heap_bitmap_alloc(kheap, size + extra);

      if (!addr) {
        assert(0 && "out of memory");
        return 0; // OOM
      }

      uint32_t aligned_addr = ((uint32_t)addr + PAGE_SIZE) & ~0xFFF;
      uint32_t offset = aligned_addr - (uint32_t)addr;

      // if offset is too small, we can't store adjustment info
      if (offset < sizeof(uint32_t)) {
        offset += PAGE_SIZE;
        aligned_addr += PAGE_SIZE;
      }

      // store adjustment before aligned address
      *((uint32_t *)(aligned_addr - sizeof(uint32_t))) = (uint32_t)addr;
      addr = aligned_addr;
    } else {
      addr = (uint32_t)kernel_heap_bitmap_alloc(kheap, size);
    }

    if (phys && addr) {
      // convert virtual to physical address
      *phys = virt2phys((void *)addr);
    }
  }
  serial_debug("giving them the address %x", addr);
  return (uint32_t)addr;
}

uint32_t kmalloc_a(uint32_t size) { return kmalloc_int(size, 1, 0); }

uint32_t kmalloc_p(uint32_t size, uint32_t *phys) {
  return kmalloc_int(size, 0, phys);
}

uint32_t kmalloc_ap(uint32_t size, uint32_t *phys) {
  return kmalloc_int(size, 1, phys);
}

uint32_t kmalloc(uint32_t size) { return kmalloc_int(size, 0, 0); }

void kfree(void *addr) {
  if (kheap == NULL || addr == NULL) {
    serial_debug("nothing to free");
    return; // Can't free memory before heap initialization or null pointer
  }

  // Check if this was an aligned allocation
  uint32_t addr_val = (uint32_t)addr;
  if (addr_val % PAGE_SIZE == 0) {
    // Could be aligned - check if we have adjustment stored
    uint32_t *adjustment_ptr = (uint32_t *)(addr_val - sizeof(uint32_t));
    uint32_t adjustment = *adjustment_ptr;

    // Heuristic: if adjustment looks like a valid heap address and
    // is within reasonable range, assume this was aligned
    if (adjustment < addr_val && addr_val - adjustment < 0x2000) {
      addr = (void *)adjustment;
    }
  }

  kernel_heap_bitmap_free(kheap, addr);
}