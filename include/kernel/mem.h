#include <stdint.h>
#include <kernel/atag.>
#include <kernel/list.h>
#ifndef MEM_H
#define MEM_H

#define PAGE_SIZE 4096

typedef struct {
	uint8_t allocated: 1;    //this page is allocated to something
	uint8_t kernel_page: 1;  //the page is part of the kernel
	uint32_t reserved: 30;
} page_flags_t;

typedef struct page{
	uint32_t vaddr_mapped;  //the virtual address that maps to this page
	page_flags_t flags;
	DEFINE_LINK(page);
} page_t;

