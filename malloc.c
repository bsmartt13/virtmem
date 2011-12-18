#include "memalloc.h"
//#include "../lib/kernel/list.h"
//#include "../lib/kernel/list.c"


struct list mem_list;

/* Initialize memory allocator to use 'length' 
   bytes of memory at 'base'. */
void mem_init (uint8_t *base, size_t length) {

	list_init (&mem_list);				/* initialize new list		*/
	struct free_block *initial_block;		/* new block struct		*/
	initial_block = (struct free_block *) base;
	initial_block->length = length;		/* set length of block		*/
	list_push_front(&mem_list, &(initial_block->elem)); /* push new block to front		*/
	size_t start_addr = (size_t) initial_block;
	size_t end_addr = start_addr + initial_block->length;
//	printf("init_fb starts at: %u, ends at: %u, ->len = %u\n",start_addr, end_addr, initial_block->length);
}

/* Allocate 'length' bytes of memory. */
void * mem_alloc (size_t length) {

  if (length < 12) length = 12; /* smallest allocation possible */
  struct list_elem *e;	      /* element to use for iteration */
  for (e = list_begin (&mem_list); e != list_end (&mem_list);
       e = list_next (e)) {

    struct free_block *usable_free_block = list_entry (e, struct free_block, elem);
    struct used_block *return_block;			

    if (usable_free_block->length == length + 4) {

       /* make used block, no resize */
//	printf("this block just fits, it is %u bytes.\n", length);
//	printf("the free block is %u bytes\n", usable_free_block->length);
       return_block = 
	   (struct used_block *) (usable_free_block);
	 return_block->length = length;
	 list_remove(e);
	  size_t ub_start_addr, ub_end_addr;
	  ub_start_addr = (size_t) return_block;
	  ub_end_addr = ub_start_addr + return_block->length;
  //        printf("new used blck starts at %u,and ends at %u. its length is %u\n", ub_start_addr, ub_end_addr, return_block->length);
	 return (return_block->data);
       }
	 else if (usable_free_block->length > length + 4)
       {

       /* make used block, resize free block 					*/
       /* need to subtract 4 for length field, which user doesn't want		*/
	size_t fba = (size_t) usable_free_block;
	size_t fbe = (size_t) fba + usable_free_block->length;
	fbe -= (length + 4);
	size_t uba = fbe;
//	printf("new block starting at %u\n", uba);
/*	if ((fbe - fba) < 12){
	  printf("resized block is too small, need to remove it\n");
	  printf("free block was %u\n",(fbe-fba));
	  list_remove(e);
	  printf("fuckin removed successfully\n");
	  uba = fba;
	  printf("length before: %u\n", length);
	  length += usable_free_block->length;
	  printf("length after: %u", length);
	}   */
	return_block = 
         (struct used_block *) uba;
       return_block->length = length;	/* store len of new used_block */
       usable_free_block->length -= (length + 4); /* shorten free block by len + 4 */
	fbe += usable_free_block->length;
	size_t ub_start_addr = (size_t) return_block;
	size_t ub_end_addr = ub_start_addr + return_block->length + 4;
//	printf("resized block starts at %u and now ends at %u. its length is %u\n", fba, fbe, usable_free_block->length);
//	printf("new used blck starts at %u,and now ends at %u. its length is %u\n", ub_start_addr, ub_end_addr, return_block->length);
       return (return_block->data);
       }   			
     }
  return NULL; /* no large enough free blocks */
}

/* Merge any adjacent free blocks in mem_list */
void merge_adj_blocks(){

 struct list_elem *e;
 for (e = list_begin (&mem_list); list_next(e) != list_end (&mem_list);
      e = list_next(e)){			  Iterate through list 
     look at pairs of blocks in elem_list 
    struct free_block *current_block;
    struct free_block *next_block;
    struct list_elem *next_e;
    next_e = list_next(e);
    current_block = list_entry (e, struct free_block, elem);
    next_block = list_entry(next_e, struct free_block, elem);
  //  printf("b1 len = %u, b2 len = %u\n\n", current_block->length, next_block->length);
    size_t endcur = (size_t) current_block;
    size_t startnext = (size_t) next_block;
    endcur += current_block->length;
    
    if (endcur == startnext) {
//      printf("looks like we need a merge.\n");
      current_block->length += next_block->length;
      list_remove(&next_block->elem);
    }
  }
}


/* Compare two list_elems by their address in memory	*/
/* true if a is less than b, false if a is equal    	*/
/* or greater than b					*/
bool free_block_less_func (const struct list_elem *a,
                             const struct list_elem *b,
                             void *aux) {
  struct free_block *sa;
  struct free_block *sb;
  sa = list_entry(a, struct free_block, elem);
  sb = list_entry(b, struct free_block, elem);
  size_t aa = (size_t) sa;
  size_t bb = (size_t) sb;
  return aa < bb;	

}

/* Free memory pointed to by 'ptr'. */
void mem_free (void *ptr) {
  
  /* get start address of block to free			*/
  size_t used_start_addr = (size_t) ptr;

  /* user sends in data[0], subtract 4 to find start of	*/
  /* used_block struct					*/
  used_start_addr -= 4;
  
  /* cast used block to free block			*/
  struct free_block *new_fb;
  new_fb = (struct free_block *) used_start_addr;
  size_t fb_start = (size_t) new_fb;  /* DEBUG CODE START */
  size_t fb_end = fb_start + new_fb->length + 4;
  new_fb->length += 4;  /* maybe remove */
//  printf("freeing up new block of length %u\n", new_fb->length);
//  printf("block start: %u\n block end: %u\n", fb_start, fb_end); /* DEBUG CODE END */
  list_insert_ordered(&mem_list, &(new_fb->elem), free_block_less_func, NULL);
  coalesce_free_block (&(new_fb->elem));
}

/* After freeing a used block, and inserting	*/
/* it back in the free list, check the block	*/
/* before and after it for coalescence 		*/
void
coalesce_free_block (struct list_elem *e) {

  struct list_elem *prev_elem;
  struct list_elem *next_elem;

  prev_elem = list_prev(e);
  next_elem = list_next(e);

  struct free_block *this_fb;
  this_fb = list_entry (e, struct free_block, elem);
  size_t fb_start_addr = (size_t) this_fb;
  size_t fb_end_addr = fb_start_addr + this_fb->length; /* + 4; maybe remove */

//        printf("\n");
//	printf("jumping into coalesce: \n");
  //      printf("this block starts at %u, ends at %u\n", fb_start_addr, fb_end_addr);
    //    //printf("\n");

  if (prev_elem != list_head (&mem_list)) {
//        printf("this block's prev is not the head.\n");

    /* If previous is not head of list, 	*/
    /* check coalescence with this free block 	*/

    struct free_block *prev_fb;
    prev_fb =  list_entry (prev_elem, struct free_block, elem);

    /* check end of previous free block 	*/
    /* with start of this free block		*/

    size_t prev_end_addr;
    size_t prev_start_addr = (size_t) prev_fb;
    prev_end_addr = (size_t) prev_fb;
    prev_end_addr += prev_fb->length; /* + 4; maybe remove */
//	printf("the previous block in the free list starts at %u, and ends at %u\n", prev_start_addr, prev_end_addr);
    if (prev_end_addr == fb_start_addr) {
//        printf("****** Coalescing blocks found: Prev & This ******\n");
      /* this block coalesces with previous	*/
      /* block, resize previous block and	*/
      /* remove this block from mem_list	*/
//        printf("prev block's length before coalesce: %u. We add %u to it, ", prev_fb->length, this_fb->length);
      prev_fb->length += this_fb->length;
      
      /* now we add 4, taking into account the	*/
      /* first 4 bytes of "this_fb" free_block 	*/
      /* prev_fb->length += 4;				maybe remove */
//	printf("and is now: %u\n", prev_fb->length);
      list_remove(&(this_fb->elem));
      this_fb = prev_fb;   /* maybe remove */
    }
  }

  if (next_elem != list_tail (&mem_list)) {
//        printf("this block's next is not the tail\n");
    /* If next is not tail of list,         	*/      
    /* check coalescence with this free block   */

    struct free_block *next_fb;
    next_fb = list_entry (next_elem, struct free_block, elem);

    /* check start of next free block 		*/
    /* with end of this free block		*/

    size_t next_start_addr, next_end_addr;
    next_start_addr = (size_t) next_fb;
    next_end_addr = next_start_addr + next_fb->length + 4;
//	printf("the next block in the free list starts at %u, and ends at %u\n", next_start_addr, next_end_addr);
    if (next_start_addr == fb_end_addr) {
//        printf("****** Coalescing blocks found: Next & This ******\n");

      /* this block coalesces with next		*/
      /* block, resize this block and       	*/
      /* remove next block from mem_list        */

//	printf("prev block's length before coalesce: %u. We add %u to it, ", this_fb->length, next_fb->length);
      this_fb->length += next_fb->length;
      /* now we add 4, taking into account the	*/
      /* first 4 bytes of "next_fb" free_block	*/
      /* this_fb->length += 4;				maybe remove */
//	 printf("and is now: %u\n", this_fb->length);
      list_remove(&(next_fb->elem)); 
    }
  }
}


/* Return the number of elements in the free list. */
size_t mem_sizeof_free_list (void) {
  return (list_size(&mem_list));
}


/* Dump the free list.  Implementation of this method is optional. */
void mem_dump_free_list(void) {
  struct list_elem *e;
  for (e = list_begin(&mem_list); e != list_end(&mem_list); e = list_next(e)) {
    struct free_block *fbp;
    fbp = list_entry(e, struct free_block, elem);
    //printf("%p %u\n", fbp, fbp->length); /* DEBUG start (uncomment this line for final version */
    size_t start_addr = (size_t) fbp;    
    printf("%u %u\n", start_addr, fbp->length);

  }
}

