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

static int32_t pos[2], angle;
static struct voxscape *vox;

#define COLOR_HORIZON	0xcc77ff
#define COLOR_ZENITH	0x5588cc



struct screen *init_game_screen(void)
{
	return &gamescr;
}

static int gamescr_start(void)
{
	gba_setmode(4, DISPCNT_BG2 | DISPCNT_OBJ | DISPCNT_FB1);

	vblperf_setcolor(1);

	if(!(vox = vox_create(512, 512))) {
		panic(get_pc(), "vox_create");
	}
	vox_proj(vox, 45, 1, 250);

	/*select_input(BN_DPAD | BN_LT | BN_RT);*/

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

	vox_framebuf(vox, 240, 160, fb, -1);

	update();
	draw();

	vblperf_end();
	wait_vblank();
	present(backbuf);
	vblperf_begin();
}

#define WALK_SPEED	0x40000
#define TURN_SPEED	0x100

static void update(void)
{
	int32_t fwd[2], right[2];
	uint16_t input;

	input = ~REG_KEYINPUT;

	if(input & BN_LT) angle += TURN_SPEED;
	if(input & BN_RT) angle -= TURN_SPEED;

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
	if(input & BN_RIGHT) {
		pos[0] += right[0];
		pos[1] += right[1];
	}
	if(input & BN_LEFT) {
		pos[0] -= right[0];
		pos[1] -= right[1];
	}

	vox_view(vox, pos[0], pos[1], -30, angle);
}

static void draw(void)
{
	vox_render(vox);
	vox_sky_grad(vox, COLOR_HORIZON, COLOR_ZENITH);
}

#ifdef BUILD_GBA
__attribute__((noinline, target("arm"), section(".iwram")))
#endif
static void gamescr_vblank(void)
{
	num_vbl++;
}
