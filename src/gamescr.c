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
#include "voxscape.h"
#include "data.h"

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

static uint16_t *framebuf;

static int nframes, backbuf;
static uint16_t *vram[] = { gba_vram_lfb0, gba_vram_lfb1 };

static int32_t pos[2], angle;
static struct voxscape *vox;

#define COLOR_HORIZON	192
#define COLOR_ZENITH	255

#define MAX_SPR		8
static uint16_t oam[4 * MAX_SPR];


struct screen *init_game_screen(void)
{
	return &gamescr;
}

static int gamescr_start(void)
{
	int i;

	gba_setmode(4, DISPCNT_BG2 | DISPCNT_OBJ | DISPCNT_FB1);

	vblperf_setcolor(0);

	pos[0] = pos[1] = VOX_SZ << 15;

	if(!(vox = vox_create(VOX_SZ, VOX_SZ, height_pixels, color_pixels))) {
		panic(get_pc(), "vox_create");
	}
	vox_proj(vox, 30, 2, 85);

	/* setup color image palette */
	for(i=0; i<256; i++) {
		int r = color_cmap[i * 3];
		int g = color_cmap[i * 3 + 1];
		int b = color_cmap[i * 3 + 2];
		gba_bgpal[i] = (((uint16_t)b << 7) & 0x7c00) | (((uint16_t)g << 2) & 0x3e0) | (((uint16_t)r >> 3) & 0x1f);
	}
	/*
	intr_disable();
	interrupt(INTR_HBLANK, hblank);
	REG_DISPSTAT |= DISPSTAT_IEN_HBLANK;
	unmask(INTR_HBLANK);
	intr_enable();
	*/

	spr_setup(16, 2, spr_game_pixels, spr_game_cmap);

	wait_vblank();
	spr_clear();
	spr_oam(oam, 0, 516, 0, 144, SPR_SZ16 | SPR_256COL);
	spr_oam(oam, 1, 516, 16, 144, SPR_SZ16 | SPR_256COL);
	spr_oam(oam, 2, 516, 32, 144, SPR_SZ16 | SPR_256COL);
	spr_oam(oam, 3, 520, 48, 144, SPR_SZ16 | SPR_256COL);

	spr_oam(oam, 4, 512, 176, 144, SPR_SZ16 | SPR_256COL);
	spr_oam(oam, 5, 516, 192, 144, SPR_SZ16 | SPR_256COL);
	spr_oam(oam, 6, 516, 208, 144, SPR_SZ16 | SPR_256COL);
	spr_oam(oam, 7, 516, 224, 144, SPR_SZ16 | SPR_256COL);
	dma_copy16(3, (void*)OAM_ADDR, oam, sizeof oam / 2, 0);

	nframes = 0;
	return 0;
}

static void gamescr_stop(void)
{
	/*mask(INTR_HBLANK);*/
}

static void gamescr_frame(void)
{
	backbuf = ++nframes & 1;
	framebuf = vram[backbuf];

	vox_framebuf(vox, 240, 160, framebuf, -1);

	update();
	draw();

	vblperf_end();
	wait_vblank();
	present(backbuf);

	if(!(nframes & 15)) {
		emuprint("vbl: %d", vblperf_count);
	}
#ifdef VBLBAR
	vblperf_begin();
#else
	vblperf_count = 0;
#endif
}

#define WALK_SPEED	0x40000
#define TURN_SPEED	0x200

static volatile uint16_t input;

static void update(void)
{
	int32_t fwd[2], right[2];

	if((input = read_input())) {

		if(input & BN_LEFT) {
			angle += TURN_SPEED;
		}
		if(input & BN_RIGHT) {
			angle -= TURN_SPEED;
		}

		fwd[0] = -SIN(angle);
		fwd[1] = COS(angle);
		right[0] = fwd[1];
		right[1] = -fwd[0];

		if(input & BN_UP) {
			pos[0] += fwd[0];
			pos[1] += fwd[1];
		}
		if(input & BN_DOWN) {
			pos[0] -= fwd[0];
			pos[1] -= fwd[1];
		}
		if(input & BN_RT) {
			pos[0] += right[0];
			pos[1] += right[1];
		}
		if(input & BN_LT) {
			pos[0] -= right[0];
			pos[1] -= right[1];
		}

		vox_view(vox, pos[0], pos[1], -40, angle);
	}
}

static void draw(void)
{
	dma_fill16(3, framebuf, 0, 240 * 160 / 2);
	//fillblock_16byte(framebuf, 0, 240 * 160 / 16);

	vox_render(vox);
	//vox_sky_grad(vox, COLOR_HORIZON, COLOR_ZENITH);
	//vox_sky_solid(vox, COLOR_ZENITH);
}

#define MAXBANK		0x100

ARM_IWRAM
static void gamescr_vblank(void)
{
	static int bank, bankdir, theta, scale;
	int32_t sa, ca;

	theta = -(bank << 3);
	scale = MAXBANK + (abs(bank) >> 3);
	sa = SIN(theta) / scale;
	ca = COS(theta) / scale;

	REG_BG2X = -ca * 120 - sa * 80 + (120 << 8);
	REG_BG2Y = sa * 120 - ca * 80 + (80 << 8);

	REG_BG2PA = ca;
	REG_BG2PB = sa;
	REG_BG2PC = -sa;
	REG_BG2PD = ca;

	if((input & (BN_LEFT | BN_RIGHT)) == 0) {
		if(bank) {
			bank -= bankdir << 4;
		}
	} else if(input & BN_LEFT) {
		bankdir = -1;
		if(bank > -MAXBANK) bank -= 16;
	} else if(input & BN_RIGHT) {
		bankdir = 1;
		if(bank < MAXBANK) bank += 16;
	}
}

/*
static uint16_t skygrad[] __attribute__((section(".data"))) = {

	0x662a, 0x660a, 0x660a, 0x660b, 0x660b, 0x660b, 0x660b, 0x6a0b, 0x6a0c,
	0x6a0c, 0x6a0c, 0x6a0c, 0x6a0c, 0x6a0d, 0x6a0d, 0x6a0d, 0x6a0d, 0x6a0d,
	0x6a0d, 0x6a0e, 0x6e0e, 0x6e0e, 0x6e0e, 0x6e0e, 0x6e0f, 0x6e0f, 0x6e0f,
	0x6e0f, 0x6e0f, 0x6e0f, 0x6e10, 0x6e10, 0x7210, 0x7210, 0x7210, 0x7211,
	0x7211, 0x7211, 0x71f1, 0x71f1, 0x71f2, 0x71f2, 0x71f2, 0x71f2, 0x71f2,
	0x75f2, 0x75f3, 0x75f3, 0x75f3, 0x75f3, 0x75f3, 0x75f4, 0x75f4, 0x75f4,
	0x75f4, 0x75f4, 0x75f5, 0x79f5, 0x79f5, 0x79f5, 0x79f5, 0x79f5, 0x79f6,
	0x79f6, 0x79f6, 0x79f6, 0x79f6, 0x79f7, 0x79f7, 0x79f7, 0x7df7, 0x7df7,
	0x7df7, 0x7df8, 0x7df8, 0x7df8, 0x7dd8, 0x7dd8, 0x7dd9, 0x7dd9,

	0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9,
	0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9,
	0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9,
	0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9,
	0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9,
	0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9,
	0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9,
	0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9,
	0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9, 0x7dd9
};

ARM_IWRAM
static void hblank(void)
{
	int vcount = REG_VCOUNT;
	gba_bgpal[255] = skygrad[vcount];
}
*/
