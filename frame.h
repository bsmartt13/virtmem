/* frame.h */

#include <stdint.h>
#include <stdbool.h>
#include "threads/synch.h"

/* frame table lock */
struct lock frame_table_lock;

/* frame */
struct frame
  {
    void* page;
		bool free;
    struct spt_entry* spt;
  };

/* frame table */

/* initialize the frame system */
void init_frame_table(size_t pages);

void* get_frame (struct spt_entry* p);

void free_frame (void *page);