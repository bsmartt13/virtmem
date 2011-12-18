#include "lib/kernel/hash.h"
#include "filesys/file.h"
#include "threads/synch.h"


struct spt_entry
	{
		struct file *file;
		off_t ofs;
		uint8_t *upage;
		uint32_t read_bytes;
		uint32_t zero_bytes;
		struct hash_elem hash_elem;
		struct frame* frame;
		bool writable;
	};
	
struct spt_entry* page_lookup (const void *address);
unsigned page_hash (const struct hash_elem *p_, void *aux);
bool page_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux);
