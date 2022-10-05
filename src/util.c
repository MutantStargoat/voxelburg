#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "debug.h"

#ifdef BUILD_GBA
extern char __iheap_start;
#else
#define IWRAM_POOL_SZ	32768
#define __iheap_start	iwram[0]
static char iwram[IWRAM_POOL_SZ];
#endif
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

int ispow2(unsigned int x)
{
	int cnt = 0;
	while(x) {
		if(x & 1) cnt++;
		x >>= 1;
	}
	return cnt == 1;
}



void *malloc_nf_impl(size_t sz, const char *file, int line)
{
	void *p;
	if(!(p = malloc(sz))) {
		panic(get_pc(), "%s:%d malloc %lu\n", file, line, (unsigned long)sz);
	}
	return p;
}

void *calloc_nf_impl(size_t num, size_t sz, const char *file, int line)
{
	void *p;
	if(!(p = calloc(num, sz))) {
		panic(get_pc(), "%s:%d calloc %lu\n", file, line, (unsigned long)(num * sz));
	}
	return p;
}

void *realloc_nf_impl(void *p, size_t sz, const char *file, int line)
{
	if(!(p = realloc(p, sz))) {
		panic(get_pc(), "%s:%d realloc %lu\n", file, line, (unsigned long)sz);
	}
	return p;
}

char *strdup_nf_impl(const char *s, const char *file, int line)
{
	int len;
	char *res;

	len = strlen(s);
	if(!(res = malloc(len + 1))) {
		panic(get_pc(), "%s:%d strdup\n", file, line);
	}
	memcpy(res, s, len + 1);
	return res;
}
