#pragma once
#include "libc/types.h"

/**
   https://web.archive.org/web/20160326122206/http://jamesmolloy.co.uk/tutorial_html/7.-The%20Heap.html
   Written for JamesM's kernel development tutorials.
   This array is insertion sorted - it always remains in a sorted state (between
calls). It can store anything that can be cast to a void* -- so a u32int, or any
pointer.
**/

typedef void *type_t;
typedef int8_t (*lessthan_predicate_t)(type_t, type_t);
typedef struct {
  type_t *array;
  uint32_t size;
  uint32_t max_size;
  lessthan_predicate_t less_than;
} ordered_array_t;

int8_t standard_lessthan_predicate(type_t a, type_t b);
ordered_array_t create_ordered_array(uint32_t max_size,
                                     lessthan_predicate_t less_than);
ordered_array_t place_ordered_array(void *addr, uint32_t max_size,
                                    lessthan_predicate_t less_than);
void destroy_ordered_array(ordered_array_t *array);
void insert_ordered_array(type_t item, ordered_array_t *array);
type_t lookup_ordered_array(uint32_t i, ordered_array_t *array);
void remove_ordered_array(uint32_t i, ordered_array_t *array);
