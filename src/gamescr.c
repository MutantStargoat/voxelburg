#include <stdlib.h>
#include <string.h>
#include "gbaregs.h"
#include "game.h"
#include "dma.h"
#include "util.h"
#include "intr.h"
#include "input.h"
#include "sprite.h"
#include "debug.h"

static void draw(void);
static void vblank(void);

static int nframes, num_vbl, backbuf;
static uint16_t *vram[] = { (uint16_t*)VRAM_LFB_FB0_ADDR, (uint16_t*)VRAM_LFB_FB1_ADDR };
static uint16_t bnstate;


void gamescr(void)
{
	int i;

	REG_DISPCNT = 4 | DISPCNT_BG2 | DISPCNT_OBJ | DISPCNT_FB1;

	vblperf_setcolor(0xff);//192);

	fillblock_16byte(vram[0], 0xffffffff, 240 * 160 / 16);
	fillblock_16byte(vram[1], 0xffffffff, 240 * 160 / 16);

	mask(INTR_VBLANK);
	screen_vblank = vblank;
	unmask(INTR_VBLANK);

	nframes = 0;
	for(;;) {
		backbuf = ++nframes & 1;

		bnstate = ~REG_KEYINPUT;

		draw();

		vblperf_end();
		wait_vblank();
		present(backbuf);
		vblperf_begin();
	}
}

static void draw(void)
{
	int i, j;
	uint16_t pix;
	uint16_t *fb = vram[backbuf];

	for(i=0; i<160; i++) {
		for(j=0; j<240/2; j++) {
			pix = ((i^j) << 1) & 0xff;
			pix |= (i^j) << 9;
			*fb++ = pix;
		}
	}
}

__attribute__((noinline, target("arm"), section(".iwram")))
static void vblank(void)
{
	num_vbl++;
}
