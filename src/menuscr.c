#include "game.h"
#include "data.h"
#include "gba.h"
#include "util.h"
#include "dma.h"
#include "input.h"
#include "sprite.h"
#include "debug.h"
#include "scoredb.h"

enum {
	MENU_START,
	MENU_CTRL,
	MENU_COLORS,
	MENU_SCORE,

	NUM_MENU_ITEMS
};

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

static int num_vbl;
static int cur_x, cur_y;
static int sel;
static int running;

static const short pos[][2] = {{76, 88}, {87, 108}, {29, 128}, {75, 148}};
#define CUR_XOFFS	16

struct screen *init_menu_screen(void)
{
	return &menuscr;
}

static void setup_palette(void)
{
	int i;
	unsigned char *cmap = gba_colors ? menuscr_gba_cmap : menuscr_cmap;

	for(i=0; i<256; i++) {
		int r = cmap[i * 3];
		int g = cmap[i * 3 + 1];
		int b = cmap[i * 3 + 2];
		gba_bgpal[i] = RGB555(r, g, b);
	}
}

static int menuscr_start(void)
{
	gba_setmode(4, DISPCNT_BG2 | DISPCNT_OBJ);
	dma_copy16(3, gba_vram_lfb0, menuscr_pixels, 240 * 160 / 2, 0);

	setup_palette();

	spr_setup(16, 4, spr_menu_pixels, spr_menu_cmap);

	wait_vblank();
	spr_clear();

	cur_x = pos[sel][0] - CUR_XOFFS;
	cur_y = pos[sel][1];

	running = 1;
	return 0;
}

static void menuscr_stop(void)
{
	running = 0;
	wait_vblank();
	spr_clear();
}

static void menuscr_frame(void)
{
	update_keyb();

	if(KEYPRESS(BN_START)) {
		change_screen(find_screen("game"));
		return;
	}

	if(KEYPRESS(BN_A)) {
		switch(sel) {
		case MENU_START:
			change_screen(find_screen("game"));
			return;
		case MENU_CTRL:
			change_screen(find_screen("controls"));
			return;
		default:
			break;
		}
	}

	if(KEYPRESS(BN_DOWN) && sel <= NUM_MENU_ITEMS) {
		cur_x = pos[++sel][0] - CUR_XOFFS;
		cur_y = pos[sel][1];
	}
	if(KEYPRESS(BN_UP) && sel > 0) {
		cur_x = pos[--sel][0] - CUR_XOFFS;
		cur_y = pos[sel][1];
	}
	if((KEYPRESS(BN_LEFT) || KEYPRESS(BN_RIGHT)) && sel == MENU_COLORS) {
		gba_colors ^= 1;
		setup_palette();
		scores[10].score = (scores[10].score & ~1) | (gba_colors & 1);
		save_scores();
	}

	wait_vblank();
}

static const int curspr[] = {
	SPRID(0, 0), SPRID(32, 0), SPRID(64, 0), SPRID(96, 0),
	SPRID(0, 16), SPRID(32, 16), SPRID(64, 16), SPRID(96, 16)
};

#define MENU_COLOR_FBPTR		(void*)(VRAM_LFB_FB0_ADDR + 117 * 240)

static void menuscr_vblank(void)
{
	int anm, frm;
	unsigned int flags = SPR_SZ32 | SPR_HRECT | SPR_256COL;
	void *src;

	if(!running) return;

	anm = (num_vbl++ >> 3) & 0xf;
	frm = anm & 7;

	if(anm >= 8) flags |= SPR_VFLIP;

	spr_oam(0, 0, curspr[frm], cur_x - 16, cur_y - 8, flags);

	src = menuscr_pixels + (gba_colors ? 160 : 117) * 240;
	dma_copy32(3, MENU_COLOR_FBPTR, src, 16 * 240 / 4, 0);
}
