#include <math.h>
#include "gbaregs.h"
#include "intr.h"
#include "debug.h"
#include "game.h"
#include "maxmod.h"

static void vblank(void);
static void nopfunc(void);

int main(void)
{
	int i;
	volatile uint16_t *cptr;

	intr_init();

	REG_WAITCNT = WAITCNT_PREFETCH | WAITCNT_ROM_2_1;

	cptr = (uint16_t*)CRAM_BG_ADDR;
	for(i=0; i<256; i++) {
		int c = i >> 3;
		*cptr++ = c | ((c >> 1) << 10);
	}

#ifndef NOSOUND
	mmInitDefault(sound_data, 8);
	mmStart(MOD_POPCORN, MM_PLAY_LOOP);
#endif

	screen_vblank = nopfunc;

	intr_disable();
	interrupt(INTR_VBLANK, vblank);
	REG_DISPSTAT |= DISPSTAT_IEN_VBLANK;
	unmask(INTR_VBLANK);

	intr_enable();
	gamescr();

	for(;;);
	return 0;
}

static void vblank(void)
{
	vblperf_count++;

	screen_vblank();

#ifndef NOSOUND
	mmVBlank();
	mmFrame();
#endif
}

static void nopfunc(void)
{
}
