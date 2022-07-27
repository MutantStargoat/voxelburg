#include "util.h"
#include "debug.h"

extern char __iheap_start;
static char *top = &__iheap_start;

int iwram_brk(void *addr)
{
	if((char*)addr < &__iheap_start) {
		addr = &__iheap_start;
	}
	if(addr > get_sp()) {
		/*return -1;*/
		panic(get_pc(), "iwram_brk (%p) >= sp", addr);
	}
	top = addr;
	return 0;
}

void *iwram_sbrk(intptr_t delta)
{
	void *prev = top;
	iwram_brk(top + delta);
	return prev;
}
