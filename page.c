#include "lib/kernel/hash.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "vm/page.h"
#include "threads/vaddr.h"
#include "vm/frame.h"
#include "lib/string.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"

static bool install_page (void *upage, void *kpage, bool writable);

void
load_spt_entry (void* addr) {
    struct spt_entry* spt = page_lookup(addr);
      if (spt == NULL) thread_exit ();

      /* Get a page of memory. */
      uint8_t *kpage = get_frame (spt);
      if (kpage == NULL) 
            thread_exit ();

      /* Load this page. */
      if (file_read_at (spt->file, kpage, spt->read_bytes, spt->ofs) != (int) spt->read_bytes)
        thread_exit ();
      memset (kpage + spt->read_bytes, 0, spt->zero_bytes);

      /* Add the page to the process's address space. */
      if (!install_page (spt->upage, kpage, spt->writable)) 
            thread_exit ();
}

/* Lookup an address in the page table and returns the page if there */
struct spt_entry*
page_lookup (const void *address) {
	struct spt_entry spt;
	struct hash_elem *e;
	spt.upage = pg_round_down(address);
	lock_acquire(&thread_current()->page_table_lock);
	e = hash_find (&thread_current()->page_table, &spt.hash_elem); 
	lock_release(&thread_current()->page_table_lock);
	return e != NULL ? hash_entry (e, struct spt_entry, hash_elem) : NULL;
}

/* Returns a hash value for page p */
unsigned page_hash (const struct hash_elem *p_, void *aux UNUSED) {
	const struct spt_entry *p = hash_entry (p_, struct spt_entry, hash_elem); 
	return hash_bytes (&p->upage, sizeof p->upage);
}

/* Returns true if page a precedes page b */
bool page_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED) {
	const struct spt_entry *a = hash_entry (a_, struct spt_entry, hash_elem); 
	const struct spt_entry *b = hash_entry (b_, struct spt_entry, hash_elem);
	return a->upage < b->upage;
}

static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page_no_spt (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}

void
grow_thread_stack (void *fault_addr, struct spt_entry *new_page) {

  void *pa_fault;  /* points to a byte alligned page to be allocated. */
  
  if (new_page == NULL) {
    new_page = malloc (sizeof (struct spt_entry));
    void *kpage_ptr = get_frame (new_page);
    if (kpage_ptr == NULL)
      thread_exit ();

  pa_fault = pg_round_down (fault_addr);
  if (!install_page (pa_fault, kpage_ptr, true)) {
    thread_exit ();
  } 
  hash_insert (&thread_current ()->page_table, &new_page->hash_elem);
  }
}
