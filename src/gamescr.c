#include <stdlib.h>
#include <string.h>
#include "gbaregs.h"
#include "game.h"
#include "dma.h"
#include "util.h"
#include "intr.h"
#include "input.h"
#include "player.h"
#include "gba.h"
#include "sprite.h"
#include "debug.h"
#include "level.h"
#include "xgl.h"
#include "polyfill.h"

static int gamescr_start(void);
static void gamescr_stop(void);
static void gamescr_frame(void);
static void gamescr_vblank(void);

static void update(void);
static void draw(void);

static struct screen gamescr = {
	"game",
	gamescr_start,
	gamescr_stop,
	gamescr_frame,
	gamescr_vblank
};

static int nframes, num_vbl, backbuf;
static uint16_t *vram[] = { gba_vram_lfb0, gba_vram_lfb1 };

static const char *testlvl =
	"################\n"
	"################\n"
	"################\n"
	"################\n"
	"################\n"
	"#######   s#####\n"
	"####### ########\n"
	"#######    #####\n"
	"######     #####\n"
	"######     #####\n"
	"######     #####\n"
	"###### ### #####\n"
	"###### ### #####\n"
	"######     #####\n"
	"######## #######\n"
	"################\n"
	"################\n"
	"################\n"
	"################\n"
	"################\n";

static struct xvertex tm_floor[] __attribute__((section(".rodata"))) = {
	{0x10000, -0x10000, 0x10000,	0, 0x10000, 0,	210},
	{-0x10000, -0x10000, 0x10000,	0, 0x10000, 0,	210},
	{-0x10000, -0x10000, -0x10000,	0, 0x10000, 0,	210},
	{0x10000, -0x10000, -0x10000,	0, 0x10000, 0,	210}
};


static struct level *lvl;

static struct player player;


struct screen *init_game_screen(void)
{
	return &gamescr;
}

static int gamescr_start(void)
{
	int i;
	uint16_t *cmap;

	gba_setmode(4, DISPCNT_BG2 | DISPCNT_OBJ | DISPCNT_FB1);

	vblperf_setcolor(1);

	lvl = init_level(testlvl);

	xgl_init();

	init_player(&player, lvl);
	player.phi = 0x100;

	cmap = gba_bgpal;
	*cmap++ = 0;
	for(i=1; i<255; i++) {
		*cmap++ = rand();
	}
	*cmap = 0xffff;


	select_input(BN_DPAD | BN_A | BN_B);

	nframes = 0;
	return 0;
}

static void gamescr_stop(void)
{
}

static void gamescr_frame(void)
{
	unsigned char *fb;

	backbuf = ++nframes & 1;
	fb = (unsigned char*)vram[backbuf];

	polyfill_framebuffer(fb, 240, 160);
	fillblock_16byte(fb, 0, 240 * 160 / 16);

	update();
	draw();

	vblperf_end();
	wait_vblank();
	present(backbuf);
	vblperf_begin();
}

static void update(void)
{
	uint16_t bnstate;

	bnstate = get_input();

	player_input(&player, bnstate);

	upd_vis(lvl, &player);
}

static void draw(void)
{
	int i, x, y;
	struct cell *cell;

	xgl_load_identity();
#ifndef BUILD_GBA
	xgl_translate(0, 0, view_zoom);
#endif
	xgl_rotate_x(player.phi);
	xgl_rotate_y(player.theta);
	xgl_translate(player.x, 0, player.y);

	for(i=0; i<lvl->numvis; i++) {
		cell = lvl->vis[i];

		x = (int32_t)(cell->x - player.cx) << 17;
		y = -(int32_t)(cell->y - player.cy) << 17;

		xgl_push_matrix();
		xgl_translate(x, 0, y);
		xgl_index(i + 1);
		xgl_draw(XGL_QUADS, tm_floor, sizeof tm_floor / sizeof *tm_floor);
		xgl_pop_matrix();
	}
}

#ifdef BUILD_GBA
__attribute__((noinline, target("arm"), section(".iwram")))
#endif
static void gamescr_vblank(void)
{
	num_vbl++;
}
