#include "gbaregs.h"
#include "game.h"
#include "gba.h"
#include "dma.h"
#include "sprite.h"
#include "util.h"
#include "xgl.h"
#include "polyfill.h"
#include "timer.h"
#include "data.h"
#include "meshdata.h"
#include "debug.h"

static int logoscr_start(void);
static void logoscr_stop(void);
static void logoscr_frame(void);
static void logoscr_vblank(void);

static struct screen logoscr = {
	"logo",
	logoscr_start,
	logoscr_stop,
	logoscr_frame,
	logoscr_vblank
};

static uint16_t *framebuf;
static int nframes, backbuf;
static uint16_t *vram[] = { gba_vram_lfb0, gba_vram_lfb1 };

static int nfaces;
static struct {	int32_t x, y; } *pos;

#define MAX_SPR	4
static uint16_t oam[4 * MAX_SPR];

static int16_t sprmat[4];


struct screen *init_logo_screen(void)
{
	return &logoscr;
}


static int logoscr_start(void)
{
	int i;
	unsigned int sprflags;

	gba_setmode(4, DISPCNT_BG2 | DISPCNT_OBJ);

	gba_bgpal[0] = 0;
	gba_bgpal[0xff] = 0xffff;

	xgl_viewport(0,  0, 240, 160);

	nfaces = sizeof logomesh / sizeof *logomesh / 3;
	pos = malloc_nf(nfaces * sizeof *pos);

	for(i=0; i<nfaces; i++) {
		pos[i].x = (rand() & 0xffff) - 0x8000;
		pos[i].y = (rand() & 0xffff) - 0x8000;
		pos[i].x += pos[i].x << 4;
		pos[i].y += pos[i].y << 4;
	}

	spr_setup(16, 8, spr_logo_pixels, spr_logo_cmap);
	/* setup blank glint palette */
	for(i=0; i<192; i++) {
		gba_objpal[i + 64] = 0xffff;
	}

	REG_BLDCNT = BLDCNT_DARKEN | BLDCNT_A_OBJ;
	REG_BLDY = 0;
	sprflags = SPR_SZ64 | SPR_HRECT | SPR_256COL | SPR_DBLSZ | SPR_ROTSCL | SPR_ROTSCL_SEL(0);
	spr_oam(oam, 0, SPRID(0, 0), 24-32, 125-16, sprflags);
	spr_oam(oam, 1, SPRID(64, 0), 64+24-32, 125-16, sprflags);
	spr_oam(oam, 2, SPRID(0, 32), 128+24-32, 125-16, sprflags);

	sprmat[0] = sprmat[3] = 0x100;
	sprmat[1] = sprmat[2] = 0;
	spr_transform(oam, 0, sprmat);

	nframes = 0;
	reset_msec_timer();
	return 0;
}

static void logoscr_stop(void)
{
	REG_BLDCNT = 0;
	REG_BLDY = 15;
}

static int32_t tm, tgoat, tname, tout;

static void logoscr_frame(void)
{
	int i;
	struct xvertex *vptr;
	int32_t x, y;

	tgoat = 0x10000 - tm;
	if(tgoat < 0) tgoat = 0;

	if(tm > 0x20000) {
		tout = (tm - 0x20000) * 5 / 3;

		if(tout > X_HPI + 0x1800) {
			tout = X_HPI + 0x1800;
			change_screen(find_screen("menu"));
			return;
		}
	}

	backbuf = ++nframes & 1;
	framebuf = vram[backbuf];

	polyfill_framebuffer(framebuf, 240, 160);
	fillblock_16byte(framebuf, 0, 240 * 160 / 16);

	xgl_load_identity();
	xgl_scale(0x10000, 0xcccc, 0x10000);
	xgl_translate(0, 0, (14 << 16) - (tout << 2));
	xgl_rotate_x(-X_HPI);
	xgl_rotate_y(tout);
	xgl_rotate_z(tout);

	xgl_index(0xff);
	vptr = logomesh;
	for(i=0; i<nfaces; i++) {

		x = (pos[i].x * (tgoat >> 8)) >> 8;
		y = (pos[i].y * (tgoat >> 8)) >> 8;

		xgl_push_matrix();
		xgl_translate(x, 0, y);
		xgl_draw(XGL_TRIANGLES, vptr, 3);
		xgl_pop_matrix();

		vptr += 3;
	}

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

ARM_IWRAM
static void logoscr_vblank(void)
{
	tm = timer_msec << 5;

	tname = 0x10000 - (tm - 0x10000);
	if(tname < 0) tname = 0;
	if(tname > 0x10000) tname = 0x10000;

	REG_BLDY = tname >> 12;

	if(tm > 0x20000) {
		if(sprmat[0] > 0) {
			sprmat[0] -= 2;
		}
		spr_transform(oam, 0, sprmat);
		REG_BLDY = (0x100 - sprmat[0]) >> 4;
		REG_BLDCNT = BLDCNT_DARKEN | BLDCNT_A_OBJ | BLDCNT_A_BG2;
	}

	dma_copy32(3, (void*)OAM_ADDR, oam, MAX_SPR * 2, 0);
}
