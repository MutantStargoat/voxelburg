#include "game.h"
#include "data.h"
#include "gba.h"
#include "util.h"
#include "dma.h"
#include "input.h"
#include "debug.h"

static int menuscr_start(void);
static void menuscr_stop(void);
static void menuscr_frame(void);
static void menuscr_vblank(void);

static struct screen menuscr = {
	"menu",
	menuscr_start,
	menuscr_stop,
	menuscr_frame,
	menuscr_vblank
};

struct screen *init_menu_screen(void)
{
	return &menuscr;
}

static int menuscr_start(void)
{
	gba_setmode(3, DISPCNT_BG2);
	dma_copy16(3, gba_vram_lfb0, menuscr_pixels, 240 * 160, 0);
	return 0;
}

static void menuscr_stop(void)
{
}

static void menuscr_frame(void)
{
	update_keyb();

	if(KEYPRESS(BN_START)) {
		change_screen(find_screen("game"));
	}
}

static void menuscr_vblank(void)
{
}
