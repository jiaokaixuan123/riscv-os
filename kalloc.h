
#ifndef KALLOC_H
#define KALLOC_H

#include "types.h"

void   pmm_init(void);
void*  alloc_page(void);
void   free_page(void* page);
// optional; returns first page (not guaranteed contiguous in this simple impl)
void*  alloc_pages(int n);

// statistics
uint64 kfree_total(void);

#endif
