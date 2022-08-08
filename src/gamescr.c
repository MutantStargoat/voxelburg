#include <stdlib.h>
#include <string.h>
#include "gbaregs.h"
#include "game.h"
#include "dma.h"
#include "util.h"
#include "intr.h"
#include "input.h"
#include "player.h"
#include "sprite.h"
#include "debug.h"
#include "level.h"
#include "xgl.h"
#include "polyfill.h"

static void update(void);
static void draw(void);
static void vblank(void);

static int nframes, num_vbl, backbuf;
static uint16_t *vram[] = { (uint16_t*)VRAM_LFB_FB0_ADDR, (uint16_t*)VRAM_LFB_FB1_ADDR };

static const char *testlvl =
	"########\n"
	"###   s#\n"
	"### ####\n"
	"###    #\n"
	"##     #\n"
	"##     #\n"
	"##     #\n"
	"## ### #\n"
	"## ### #\n"
	"##     #\n"
	"#### ###\n"
	"########\n";

static struct xvertex tm_floor[] __attribute__((section(".rodata"))) = {
	{0x10000, -0x10000, 0x10000,	0, 0x10000, 0,	210},
	{-0x10000, -0x10000, 0x10000,	0, 0x10000, 0,	210},
	{-0x10000, -0x10000, -0x10000,	0, 0x10000, 0,	210},
	{0x10000, -0x10000, -0x10000,	0, 0x10000, 0,	210}
};


static struct level *lvl;

static struct player player;


void gamescr(void)
{
	unsigned char *fb;

	REG_DISPCNT = 4 | DISPCNT_BG2 | DISPCNT_OBJ | DISPCNT_FB1;

	vblperf_setcolor(0xff);

	lvl = init_level(testlvl);

	xgl_init();

	memset(&player, 0, sizeof player);
	player.y = 0x60000;

	select_input(BN_DPAD | BN_A | BN_B);

	mask(INTR_VBLANK);
	screen_vblank = vblank;
	unmask(INTR_VBLANK);

	nframes = 0;
	for(;;) {
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
}

static void update(void)
{
	uint16_t bnstate;

	bnstate = get_input();

	player_input(&player, bnstate);
}

static void draw(void)
{
	xgl_load_identity();
	xgl_rotate_x(player.phi);
	xgl_rotate_y(player.theta);
	xgl_translate(player.x, 0, player.y);

	xgl_draw(XGL_QUADS, tm_floor, sizeof tm_floor / sizeof *tm_floor);
}

__attribute__((noinline, target("arm"), section(".iwram")))
static void vblank(void)
{
	num_vbl++;
}
