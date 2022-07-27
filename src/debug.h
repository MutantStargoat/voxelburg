#ifndef DEBUG_H_
#define DEBUG_H_

#include <stdarg.h>
#include "util.h"

extern unsigned char font_8x8[];

extern uint16_t vblperf_color[];
uint16_t *vblperf_palptr;
volatile int vblperf_count;

void vblperf_setcolor(int palidx);

#ifdef VBLBAR
#define vblperf_begin()	\
	do { \
		*vblperf_palptr = 0; \
		vblperf_count = 0; \
	} while(0)

#define vblperf_end() \
	do { \
		*vblperf_palptr = vblperf_color[vblperf_count]; \
	} while(0)
#else
#define vblperf_begin()
#define vblperf_end()
#endif

void panic(void *pc, const char *fmt, ...) __attribute__((noreturn));

void dbg_drawglyph(int x, int y, int c);
int dbg_drawstr(int x, int y, const char *fmt, ...);
int dbg_vdrawstr(int x, int y, const char *fmt, va_list ap);

void emuprint(const char *fmt, ...);

#endif	/* DEBUG_H_ */
