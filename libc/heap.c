#include "libc/heap.h"
#include "libc/page.h"
// https://web.archive.org/web/20160326122206/http://jamesmolloy.co.uk/tutorial_html/7.-The%20Heap.html

#undef serial_debug
#define serial_debug(...)

// end is defined in the linker script.
// extern uint32_t end;
uint32_t placement_address = PLACEMENT_ADDRESS; //(uint32_t)&end;
extern page_directory_t *kernel_directory;
heap_t *kheap = 0;

uint32_t kmalloc_int(uint32_t sz, int align, uint32_t *phys) {
  serial_debug("[kmalloc_int] requested size: %d bytes, align: %d", sz, align);

  if (kheap != 0) {
    void *addr = alloc(sz, (int8_t)align, kheap);
    if (phys != 0) {
      page_t *page = get_page((uint32_t)addr, 0, kernel_directory);
      *phys = page->frame * 0x1000 + ((uint32_t)addr & 0xFFF);
      serial_debug("[kmalloc_int] allocated at: %x, physical: %x", addr,
                   (void *)(*phys));
    } else {
      serial_debug("[kmalloc_int] allocated at: %x", addr);
    }
    return (uint32_t)addr;
  } else {
    if (align == 1 && (placement_address & 0xFFFFF000)) {
      serial_debug("[kmalloc_int] aligning placement address from: %x",
                   (void *)placement_address);
      placement_address &= 0xFFFFF000;
      placement_address += 0x1000;
      serial_debug("[kmalloc_int] new aligned placement address: %x",
                   (void *)placement_address);
    }
    if (phys) {
      *phys = placement_address;
      serial_debug("[kmalloc_int] physical address set to: %x",
                   (void *)(*phys));
    }
    uint32_t tmp = placement_address;
    placement_address += sz;
    serial_debug("[kmalloc_int] returning allocated address: %x", (void *)tmp);
    return tmp;
  }
}

void kfree(void *p) { free(p, kheap); }

uint32_t kmalloc_a(uint32_t sz) { return kmalloc_int(sz, 1, 0); }

uint32_t kmalloc_p(uint32_t sz, uint32_t *phys) {
  return kmalloc_int(sz, 0, phys);
}

uint32_t kmalloc_ap(uint32_t sz, uint32_t *phys) {
  return kmalloc_int(sz, 1, phys);
}

uint32_t kmalloc(uint32_t sz) { return kmalloc_int(sz, 0, 0); }

static void expand(uint32_t new_size, heap_t *heap) {
  assert(new_size > (heap->end_address - heap->start_address));

  // get the nearest following page boundary
  if ((new_size & 0xFFFFF000) != 0) {
    new_size &= 0xFFFFF000;
    new_size += 0x1000;
  }

  assert((heap->start_address + new_size) <= heap->max_address);

  // this should always be on a page boundary.
  uint32_t old_size = heap->end_address - heap->start_address;

  uint32_t i = old_size;
  while (i < new_size) {
    alloc_frame(get_page(heap->start_address + i, 1, kernel_directory),
                (heap->supervisor) ? 1 : 0, (heap->readonly) ? 0 : 1);
    i += 0x1000 /* page size */;
  }
  heap->end_address = heap->start_address + new_size;
}

static uint32_t contract(uint32_t new_size, heap_t *heap) {
  assert(new_size < heap->end_address - heap->start_address);

  // get the nearest following page boundary
  if (new_size & 0x1000) {
    new_size &= 0x1000;
    new_size += 0x1000;
  }

  if (new_size < HEAP_MIN_SIZE)
    new_size = HEAP_MIN_SIZE;

  uint32_t old_size = heap->end_address - heap->start_address;
  uint32_t i = old_size - 0x1000;
  while (new_size < i) {
    free_frame(get_page(heap->start_address + i, 0, kernel_directory));
    i -= 0x1000; /* page size */
  }

  heap->end_address = heap->start_address + new_size;
  return new_size;
}

static int32_t find_smallest_hole(uint32_t size, int8_t page_align,
                                  heap_t *heap) {
  uint32_t iterator = 0;
  while (iterator < heap->index.size) {
    header_t *header = (header_t *)lookup_ordered_array(iterator, &heap->index);
    if (page_align > 0) {
      // page-align the starting point of this header
      uint32_t location = (uint32_t)header;
      int32_t offset = 0;
      if ((location + sizeof(header_t)) & (0xFFFFF000 != 0))
        offset =
            0x1000 /* page size */ - (location + sizeof(header_t)) % 0x1000;
      int32_t hole_size = (int32_t)header->size - offset;

      if (hole_size >= (int32_t)size)
        break;
    } else if (header->size >= size)
      break;
    iterator++;
  }

  if (iterator == heap->index.size)
    return -1; // we got to the end and didnt find anything.
  else
    return iterator;
}

static int8_t header_t_less_than(void *a, void *b) {
  return (((header_t *)a)->size < ((header_t *)b)->size) ? 1 : 0;
}

heap_t *create_heap(uint32_t start, uint32_t end_addr, uint32_t max,
                    int8_t supervisor, int8_t readonly) {
  heap_t *heap = (heap_t *)kmalloc(sizeof(heap_t));

  // are they page aligned?
  assert(start % 0x1000 == 0);
  assert(end_addr % 0x1000 == 0);

  heap->index =
      place_ordered_array((void *)start, HEAP_INDEX_SIZE, &header_t_less_than);

  // shift the start address forward to resemble where we can start putting
  // data
  start += sizeof(type_t) * HEAP_INDEX_SIZE;

  // page align
  if ((start & 0xFFFFF000) != 0) {
    start &= 0xFFFFF000;
    start += 0x1000;
  }

  heap->start_address = start;
  heap->end_address = end_addr;
  heap->max_address = max;
  heap->supervisor = supervisor;
  heap->readonly = readonly;

  // we start off with one large hole in the index
  header_t *hole = (header_t *)start;
  hole->size = end_addr - start;
  hole->magic = HEAP_MAGIC;
  hole->is_hole = 1;
  insert_ordered_array((void *)hole, &heap->index);

  return heap;
}

void *alloc(uint32_t size, int8_t page_align, heap_t *heap) {

  // make sure we take the size of header/footer into account
  uint32_t new_size = size + sizeof(header_t) + sizeof(footer_t);
  // find the smallest hole that will fit.
  int32_t iterator = find_smallest_hole(new_size, page_align, heap);

  if (iterator == -1) {
    uint32_t old_length = heap->end_address - heap->start_address;
    uint32_t old_end_address = heap->end_address;

    // we need to allocate some more space
    expand(old_length + new_size, heap);
    uint32_t new_length = heap->end_address - heap->start_address;

    // find the endmost header (not endmost in size, but in location)
    iterator = 0;
    // vars to hold the index of, and value of, the endmost header found so far
    uint32_t idx = -1;
    uint32_t value = 0x0;
    while (iterator < (int32_t)heap->index.size) {
      uint32_t tmp = (uint32_t)lookup_ordered_array(iterator, &heap->index);
      if (tmp > value) {
        value = tmp;
        idx = iterator;
      }
      iterator++;
    }

    // if we didn't find ANY headers, we need to add one
    if (idx == (uint32_t)-1) {
      header_t *header = (header_t *)old_end_address;
      header->magic = HEAP_MAGIC;
      header->size = new_length - old_length;
      header->is_hole = 1;
      footer_t *footer =
          (footer_t *)(old_end_address + header->size - sizeof(footer_t));
      footer->magic = HEAP_MAGIC;
      footer->header = header;
      insert_ordered_array((void *)header, &heap->index);
    } else {
      // the last header needs adjusting
      header_t *header = lookup_ordered_array(idx, &heap->index);
      header->size += new_length - old_length;
      // rewrite the footer.
      footer_t *footer =
          (footer_t *)((uint32_t)header + header->size - sizeof(footer_t));
      footer->header = header;
      footer->magic = HEAP_MAGIC;
    }
    // we now have enough space. Recurse, and call the function again.
    return alloc(size, page_align, heap);
  }

  header_t *orig_hole_header =
      (header_t *)lookup_ordered_array(iterator, &heap->index);
  uint32_t orig_hole_pos = (uint32_t)orig_hole_header;
  uint32_t orig_hole_size = orig_hole_header->size;
  // here we work out if we should split the hole we found into two parts.
  // is the original hole size - requested hole size less than the overhead for
  // adding a new hole?
  if (orig_hole_size - new_size < sizeof(header_t) + sizeof(footer_t)) {
    // then just increase the requested size to the size of the hole we found.
    size += orig_hole_size - new_size;
    new_size = orig_hole_size;
  }

  // if we need to page-align the data, do it now and make a new hole in front
  // of our block.
  if (page_align && orig_hole_pos & 0xFFFFF000) {
    uint32_t new_location = orig_hole_pos + 0x1000 /* page size */ -
                            (orig_hole_pos & 0xFFF) - sizeof(header_t);
    header_t *hole_header = (header_t *)orig_hole_pos;
    hole_header->size =
        0x1000 /* page size */ - (orig_hole_pos & 0xFFF) - sizeof(header_t);
    hole_header->magic = HEAP_MAGIC;
    hole_header->is_hole = 1;
    footer_t *hole_footer =
        (footer_t *)((uint32_t)new_location - sizeof(footer_t));
    hole_footer->magic = HEAP_MAGIC;
    hole_footer->header = hole_header;
    orig_hole_pos = new_location;
    orig_hole_size = orig_hole_size - hole_header->size;
  } else {
    // else we don't need this hole any more, delete it from the index.
    remove_ordered_array(iterator, &heap->index);
  }

  // overwrite the original header
  header_t *block_header = (header_t *)orig_hole_pos;
  block_header->magic = HEAP_MAGIC;
  block_header->is_hole = 0;
  block_header->size = new_size;
  // ...and the footer
  footer_t *block_footer =
      (footer_t *)(orig_hole_pos + sizeof(header_t) + size);
  block_footer->magic = HEAP_MAGIC;
  block_footer->header = block_header;

  // we may need to write a new hole after the allocated block.
  // we do this only if the new hole would have positive size...
  if (orig_hole_size - new_size > 0) {
    header_t *hole_header = (header_t *)(orig_hole_pos + sizeof(header_t) +
                                         size + sizeof(footer_t));
    hole_header->magic = HEAP_MAGIC;
    hole_header->is_hole = 1;
    hole_header->size = orig_hole_size - new_size;
    footer_t *hole_footer =
        (footer_t *)((uint32_t)hole_header + orig_hole_size - new_size -
                     sizeof(footer_t));
    if ((uint32_t)hole_footer < heap->end_address) {
      hole_footer->magic = HEAP_MAGIC;
      hole_footer->header = hole_header;
    }
    // put the new hole in the index;
    insert_ordered_array((void *)hole_header, &heap->index);
  }

  return (void *)((uint32_t)block_header + sizeof(header_t));
}

void free(void *p, heap_t *heap) {
  if (p == 0)
    return;

  header_t *header = (header_t *)((uint32_t)p - sizeof(header_t));
  footer_t *footer =
      (footer_t *)((uint32_t)header + header->size - sizeof(footer_t));

  assert(header->magic == HEAP_MAGIC);
  assert(footer->magic == HEAP_MAGIC);

  header->is_hole = 1;

  char do_add = 1; // add to free-holes index

  // unify left
  // if the thing immediately to the left of us is a footer...
  footer_t *test_footer = (footer_t *)((uint32_t)header - sizeof(footer_t));
  if (test_footer->magic == HEAP_MAGIC && test_footer->header->is_hole == 1) {
    uint32_t cache_size = header->size; // cache our current size
    header = test_footer->header;       // rewrite our header with the new one
    footer->header = header;    // rewrite our footer to point to the new header
    header->size += cache_size; // change the size
    do_add = 0; // since this header is already in the index, we don't want to
                // add it again
  }

  // unify right
  // if the thing immediately to the right of us is a header...
  header_t *test_header = (header_t *)((uint32_t)footer + sizeof(footer_t));
  if (test_header->magic == HEAP_MAGIC && test_header->is_hole) {
    header->size += test_header->size;                 // increase our size.
    test_footer = (footer_t *)((uint32_t)test_header + // rewrite it's footer to
                                                       // point to our header
                               test_header->size - sizeof(footer_t));
    footer = test_footer;
    // find and remove this header from the index
    uint32_t iterator = 0;
    while (
        (iterator < heap->index.size) &&
        (lookup_ordered_array(iterator, &heap->index) != (void *)test_header))
      iterator++;

    // make sure we actually found the item.
    assert(iterator < heap->index.size);
    remove_ordered_array(iterator, &heap->index);
  }

  // if the footer location is the end address, we can contract
  if ((uint32_t)footer + sizeof(footer_t) == heap->end_address) {
    uint32_t old_length = heap->end_address - heap->start_address;
    uint32_t new_length =
        contract((uint32_t)header - heap->start_address, heap);
    // check how big we will be after resizing
    if (header->size - (old_length - new_length) > 0) {
      // we will still exist, so resize us
      header->size -= old_length - new_length;
      footer = (footer_t *)((uint32_t)header + header->size - sizeof(footer_t));
      footer->magic = HEAP_MAGIC;
      footer->header = header;
    } else {
      // remove us from the index
      uint32_t iterator = 0;
      while (
          (iterator < heap->index.size) &&
          (lookup_ordered_array(iterator, &heap->index) != (void *)test_header))
        iterator++;
      if (iterator < heap->index.size)
        remove_ordered_array(iterator, &heap->index);
    }
  }

  // if required, add us to the index
  if (do_add == 1)
    insert_ordered_array((void *)header, &heap->index);
}
