#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include "gbaregs.h"
#include "intr.h"
#include "debug.h"
#include "util.h"

uint16_t vblperf_color[] = {
	/* grn  blue   cyan  yellow  orng    red     purple  d.green purple ... */
	/* 60    30     20     15     12      10      8.5     7.5    ... */
	0x3e0, 0xf863, 0xffc0, 0x3ff, 0x1ff, 0x001f, 0xf81f, 0x1e0, 0xf81f, 0xf81f, 0xf81f
};

void vblperf_setcolor(int palidx)
{
	vblperf_palptr = (uint16_t*)CRAM_BG_ADDR + palidx;
}


uint32_t panic_regs[16];
void get_panic_regs(void);

void panic(void *pc, const char *fmt, ...)
{
	int y;
	va_list ap;
	uint32_t *reg;

	get_panic_regs();

	intr_disable();
	REG_DISPCNT = 4 | DISPCNT_BG2;

	set_bg_color(0, 31, 0, 0);
	set_bg_color(0xff, 31, 31, 31);

	fillblock_16byte((void*)VRAM_LFB_FB0_ADDR, 0, 240 * 160 / 16);

	fillblock_16byte((unsigned char*)VRAM_LFB_FB0_ADDR + 240 * 3, 0xffffffff, 240 / 16);
	dbg_drawstr(44, 0, " Panic at %p ", pc);

	va_start(ap, fmt);
	y = dbg_vdrawstr(0, 12, fmt, ap) + 8;
	va_end(ap);

	fillblock_16byte((unsigned char*)VRAM_LFB_FB0_ADDR + 240 * (y + 4), 0xffffffff, 240 / 16);
	y += 8;

	reg = panic_regs;
	y = dbg_drawstr(0, y, " r0 %08x  r1 %08x\n r2 %08x  r3 %08x\n r4 %08x  r5 %08x\n r6 %08x  r7 %08x\n",
			reg[0], reg[1], reg[2], reg[3], reg[4], reg[5], reg[6], reg[7]);
	y = dbg_drawstr(0, y, " r8 %08x  r9 %08x\nr10 %08x r11 %08x\n ip %08x  sp %08x\n lr %08x  pc %08x\n",
			reg[8], reg[9], reg[10], reg[11], reg[12], reg[13], reg[14], reg[15]);

	/* stop any sound/music playback */
	REG_SOUNDCNT_H = SCNT_DSA_CLRFIFO | SCNT_DSB_CLRFIFO;
	REG_TMCNT_H(1) &= ~TMCNT_EN;
	REG_DMA1CNT_H = 0;
	REG_DMA2CNT_H = 0;

	for(;;);
}

void dbg_drawglyph(int x, int y, int c)
{
	int i;
	uint16_t pp;
	unsigned char row;
	uint16_t *ptr = (uint16_t*)VRAM_LFB_FB0_ADDR + (y << 7) - (y << 3) + (x >> 1);
	unsigned char *fnt = font_8x8 + ((c & 0xff) << 3);

	for(i=0; i<8; i++) {
		row = *fnt++;
		pp = row & 0x80 ? 0xff : 0;
		*ptr++ = pp | (row & 0x40 ? 0xff00 : 0);
		pp = row & 0x20 ? 0xff : 0;
		*ptr++ = pp | (row & 0x10 ? 0xff00 : 0);
		pp = row & 0x08 ? 0xff : 0;
		*ptr++ = pp | (row & 0x04 ? 0xff00 : 0);
		pp = row & 0x02 ? 0xff : 0;
		*ptr++ = pp | (row & 0x01 ? 0xff00 : 0);
		ptr += 120 - 4;
	}
}

int dbg_vdrawstr(int x, int y, const char *fmt, va_list ap)
{
	int startx, c;
	char buf[128];
	char *ptr = buf;

	vsnprintf(buf, sizeof buf, fmt, ap);

	startx = x;
	while(*ptr) {
		if(y >= 160) break;

		c = *ptr++;
		switch(c) {
		case '\n':
			y += 8;
		case '\r':
			x = startx;
			break;

		default:
			dbg_drawglyph(x, y, c);
			x += 8;
			if(x >= 240 - 8) {
				while(*ptr && isspace(*ptr)) ptr++;
				x = 0;
				y += 8;
			}
		}
	}

	return y;
}

int dbg_drawstr(int x, int y, const char *fmt, ...)
{
	int res;
	va_list ap;

	va_start(ap, fmt);
	res = dbg_vdrawstr(x, y, fmt, ap);
	va_end(ap);
	return res;
}

#ifdef EMUBUILD
__attribute__((target("arm")))
void emuprint(const char *fmt, ...)
{
	char buf[128];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);

	asm volatile(
		"mov r0, %0\n\t"
		"swi 0xff0000\n\t" :
		: "r" (buf)
		: "r0"
	);
}
#else
void emuprint(const char *fmt, ...)
{
}
#endif
