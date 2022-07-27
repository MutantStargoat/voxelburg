#ifndef UTIL_H_
#define UTIL_H_

#include <stdint.h>

#define wait_vblank() \
	do { \
		while(REG_DISPSTAT & DISPSTAT_VBLANK); \
		while(!(REG_DISPSTAT & DISPSTAT_VBLANK)); \
	} while(0)

#define present(x) \
	do { \
		REG_DISPCNT = DISPCNT_BG2 | DISPCNT_OBJ | 4 | ((x) << 4); \
	} while(0)


#define set_bg_color(idx, r, g, b) \
	do { \
		((uint16_t*)CRAM_BG_ADDR)[idx] = (uint16_t)(r) | ((uint16_t)(g) << 5) | ((uint16_t)(b) << 10); \
	} while(0)

extern int16_t sinlut[];

#define SIN(x)	sinlut[(x) & 0xff]
#define COS(x)	sinlut[((x) + 64) & 0xff]

int iwram_brk(void *addr);
void *iwram_sbrk(intptr_t delta);

void fillblock_16byte(void *dest, uint32_t val, int count);

void *get_pc(void);
void *get_sp(void);

#endif	/* UTIL_H_ */
