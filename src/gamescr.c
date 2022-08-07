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

static struct xvertex cube[] __attribute__((section(".rodata"))) = {
	/* front */
	{-0x10000, -0x10000, -0x10000,	0, 0, -0x10000,	255},
	{0x10000, -0x10000, -0x10000,	0, 0, -0x10000,	255},
	{0x10000, 0x10000, -0x10000,	0, 0, -0x10000,	255},
	{-0x10000, 0x10000, -0x10000,	0, 0, -0x10000,	255},
	/* right */
	{0x10000, -0x10000, -0x10000,	0x10000, 0, 0,	128},
	{0x10000, -0x10000, 0x10000,	0x10000, 0, 0,	128},
	{0x10000, 0x10000, 0x10000,		0x10000, 0, 0,	128},
	{0x10000, 0x10000, -0x10000,	0x10000, 0, 0,	128},
	/* back */
	{0x10000, -0x10000, 0x10000,	0, 0, 0x10000,	200},
	{-0x10000, -0x10000, 0x10000,	0, 0, 0x10000,	200},
	{-0x10000, 0x10000, 0x10000,	0, 0, 0x10000,	200},
	{0x10000, 0x10000, 0x10000,		0, 0, 0x10000,	200},
	/* left */
	{-0x10000, -0x10000, 0x10000,	-0x10000, 0, 0,	192},
	{-0x10000, -0x10000, -0x10000,	-0x10000, 0, 0,	192},
	{-0x10000, 0x10000, -0x10000,	-0x10000, 0, 0,	192},
	{-0x10000, 0x10000, 0x10000,	-0x10000, 0, 0,	192},
	/* top */
	{-0x10000, 0x10000, -0x10000,	0, 0x10000, 0,	150},
	{0x10000, 0x10000, -0x10000,	0, 0x10000, 0,	150},
	{0x10000, 0x10000, 0x10000,		0, 0x10000, 0,	150},
	{-0x10000, 0x10000, 0x10000,	0, 0x10000, 0,	150},
	/* bottom */
	{0x10000, -0x10000, -0x10000,	0, -0x10000, 0,	210},
	{-0x10000, -0x10000, -0x10000,	0, -0x10000, 0,	210},
	{-0x10000, -0x10000, 0x10000,	0, -0x10000, 0,	210},
	{0x10000, -0x10000, 0x10000,	0, -0x10000, 0,	210}
};


static struct level *lvl;

static int32_t cam_theta, cam_phi;

void gamescr(void)
{
	unsigned char *fb;

	REG_DISPCNT = 4 | DISPCNT_BG2 | DISPCNT_OBJ | DISPCNT_FB1;

	vblperf_setcolor(0xff);

	lvl = init_level(testlvl);

	xgl_init();

	select_input(BN_DPAD);

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

	if(bnstate & KEY_UP) {
		cam_phi += 0x2000;
		if(cam_phi > X_HPI) cam_phi = X_HPI;
	}
	if(bnstate & KEY_DOWN) {
		cam_phi -= 0x2000;
		if(cam_phi < -X_HPI) cam_phi = -X_HPI;
	}
	if(bnstate & KEY_LEFT) {
		cam_theta += 0x2000;
		if(cam_theta > X_2PI) cam_theta -= X_2PI;
	}
	if(bnstate & KEY_RIGHT) {
		cam_theta -= 0x2000;
		if(cam_theta < X_2PI) cam_theta += X_2PI;
	}
}

static void draw(void)
{
	xgl_load_identity();
	//xgl_translate(0, -0x50000, 0);
	xgl_translate(0, 0, 0x80000);
	xgl_rotate_x(cam_phi);
	xgl_rotate_y(cam_theta);

	xgl_draw(XGL_QUADS, cube, sizeof cube / sizeof *cube);
}

__attribute__((noinline, target("arm"), section(".iwram")))
static void vblank(void)
{
	num_vbl++;
}
