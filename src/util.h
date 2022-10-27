#ifndef UTIL_H_
#define UTIL_H_

#include <stdlib.h>
#include <stdint.h>
#include "gba.h"

#ifdef BUILD_GBA

#define wait_vblank() \
	do { \
		while(REG_DISPSTAT & DISPSTAT_VBLANK); \
		while(!(REG_DISPSTAT & DISPSTAT_VBLANK)); \
	} while(0)

#define present(x) \
	do { \
		REG_DISPCNT = (REG_DISPCNT & 0xffef) | ((x) << 4); \
	} while(0)

#define ARM_IWRAM	__attribute__((noinline, target("arm"), section(".iwram")))

#else	/* non-GBA build */
#define wait_vblank()

void present(int buf);		/* defined in src/pc/main.c */

#define ARM_IWRAM
#endif

#define set_bg_color(idx, r, g, b) \
	gba_bgpal[idx] = (uint16_t)(r) | ((uint16_t)(g) << 5) | ((uint16_t)(b) << 10)

extern int16_t sinlut[];

#define SINLUT_BITS		8
#define SINLUT_SIZE		(1 << SINLUT_BITS)

#define SIN(angle) \
	((int32_t)sinlut[((angle) >> (16 - SINLUT_BITS)) & (SINLUT_SIZE - 1)] << 1)

#define COS(angle) \
	((int32_t)sinlut[(((angle) >> (16 - SINLUT_BITS)) + (SINLUT_SIZE / 4)) & (SINLUT_SIZE - 1)] << 1)

int iwram_brk(void *addr);
void *iwram_sbrk(intptr_t delta);

void fillblock_16byte(void *dest, uint32_t val, int count);

void *get_pc(void);
void *get_sp(void);

int ispow2(unsigned int x);

/* Non-failing versions of malloc/calloc/realloc. They never return 0, they call
 * demo_abort on failure. Use the macros, don't call the *_impl functions.
 */
#define malloc_nf(sz)	malloc_nf_impl(sz, __FILE__, __LINE__)
void *malloc_nf_impl(size_t sz, const char *file, int line);
#define calloc_nf(n, sz)	calloc_nf_impl(n, sz, __FILE__, __LINE__)
void *calloc_nf_impl(size_t num, size_t sz, const char *file, int line);
#define realloc_nf(p, sz)	realloc_nf_impl(p, sz, __FILE__, __LINE__)
void *realloc_nf_impl(void *p, size_t sz, const char *file, int line);
#define strdup_nf(s)	strdup_nf_impl(s, __FILE__, __LINE__)
char *strdup_nf_impl(const char *s, const char *file, int line);


#endif	/* UTIL_H_ */
