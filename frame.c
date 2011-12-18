#include "vm/frame.h"
#include "vm/page.h"
#include "devices/timer.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "threads/init.h"
#include "userprog/pagedir.h"
#include "lib/string.h"
#include <stdio.h>
#include <string.h>
#include "malloc.h"

static size_t TABLE_SIZE;
static struct frame *frame_list;

/* initialize the frame system */
void init_frame_table(size_t pages) {
	int i;
	TABLE_SIZE = pages;
	void *page;
	size_t size = 0;
  lock_init (&frame_table_lock);
  frame_list = malloc(sizeof(struct frame)*pages);

  page = palloc_get_page (PAL_USER);
  for (i = 0; page != NULL && i < (int) TABLE_SIZE; i++) {
    size++;
    frame_list[i].free = true;
    frame_list[i].page = page;
    frame_list[i].spt = NULL;
    page = palloc_get_page (PAL_USER);
  }
  TABLE_SIZE = size;
}

void*
get_frame (struct spt_entry* p) {
  int i;
  ASSERT (p->frame == NULL);
	struct frame *f = &frame_list[0];
	lock_acquire(&frame_table_lock);
  for (i = 0; i < (int) TABLE_SIZE && !f->free; i++) 
		f = &frame_list[i];

	if (f->free) {
		f->free = false;
		memset(f->page, 0, PGSIZE);
		f->spt = p;
		p->frame = f;
		lock_release(&frame_table_lock);
		return f->page;
	}
	lock_release(&frame_table_lock);
	return NULL;
}

void
free_frame (void* addr) {
	struct spt_entry* spt;
	struct frame* f;
	
	spt = page_lookup(addr);
	if (spt == NULL || spt->frame == NULL)
		return;
	lock_acquire(&frame_table_lock);
	f = spt->frame;
	spt->frame = NULL;
	f->free = true;
	f->spt = NULL;
	lock_release(&frame_table_lock);
}