#include "game.h"
#include "data.h"
#include "gba.h"
#include "util.h"
#include "dma.h"
#include "input.h"
#include "debug.h"

static int ctrlscr_start(void);
static void ctrlscr_stop(void);
static void ctrlscr_frame(void);
static void ctrlscr_vblank(void);

static struct screen ctrlscr = {
	"controls",
	ctrlscr_start,
	ctrlscr_stop,
	ctrlscr_frame,
	ctrlscr_vblank
};

struct screen *init_controls_screen(void)
{
	return &ctrlscr;
}

static void setup_palette(void)
{
	int i;
	unsigned char *cmap = gba_colors ? controls_gba_cmap : controls_cmap;

	for(i=0; i<256; i++) {
		int r = cmap[i * 3];
		int g = cmap[i * 3 + 1];
		int b = cmap[i * 3 + 2];
		gba_bgpal[i] = RGB555(r, g, b);
	}
}

static int ctrlscr_start(void)
{
	gba_setmode(4, DISPCNT_BG2 | DISPCNT_OBJ);
	dma_copy16(3, gba_vram_lfb0, controls_pixels, 240 * 160 / 2, 0);

	setup_palette();
	return 0;
}

static void ctrlscr_stop(void)
{
	wait_vblank();
}

static void ctrlscr_frame(void)
{
	update_keyb();

	if(KEYPRESS(BN_START) || KEYPRESS(BN_A) || KEYPRESS(BN_B)) {
		change_screen(find_screen("menu"));
		return;
	}

	wait_vblank();
}

static void ctrlscr_vblank(void)
{
}
