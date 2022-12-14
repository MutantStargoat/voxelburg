#include <stdlib.h>
#include <string.h>
#include "gbaregs.h"
#include "game.h"
#include "dma.h"
#include "util.h"
#include "intr.h"
#include "input.h"
#include "gba.h"
#include "sprite.h"
#include "timer.h"
#include "debug.h"
#include "voxscape.h"
#include "data.h"
#include "scoredb.h"

#define POS_MASK	((VOX_SZ << 16) - 1)

#define FOV		30
#define NEAR	2
#define FAR		85

#define P_RATE	250
#define E_RATE	3000
#define SHOT_TIME	50

#define ENEMY_VIS_RANGE	(2 * FAR / 3)

static int gamescr_start(void);
static void gamescr_stop(void);
static void gamescr_frame(void);
static void gamescr_vblank(void);

static int update(void);
static void draw(void);
static void victory(void);

static struct screen gamescr = {
	"game",
	gamescr_start,
	gamescr_stop,
	gamescr_frame,
	gamescr_vblank
};

#define ENEMY_ENERGY	4
struct enemy {
	struct vox_object vobj;
	short hp;
	unsigned char anm;
	signed char shot_frame;
	int last_shot;
};
#define ENEMY_VALID(e)	((e)->anm != 0)
#define NUM_SHOT_FRAMES	16

#define SFRM_LVL1	5
#define SFRM_LVL2	12

static uint16_t *framebuf;

static int nframes, backbuf;
static uint16_t *vram[] = { gba_vram_lfb0, gba_vram_lfb1 };

static int32_t pos[2], angle, horizon = 80;
static long last_shot, hitfrm;
static int hit_px, hit_py;
static int pheight;

#define COLOR_HORIZON	192
#define COLOR_ZENITH	255

#define MAX_SPR		48
static uint16_t oam[4 * MAX_SPR];
static int dynspr_base, dynspr_count;


#define MAX_ENEMIES		(255 - CMAP_SPAWN0)
static struct enemy enemies[MAX_ENEMIES];
static int num_kills, total_enemies;
static int energy;
#define MAX_ENERGY	5

static int score, gameover;
static unsigned long total_time, start_time;
static int running;

#define XFORM_PIXEL_X(x, y)	(xform_ca * (x) - xform_sa * (y) + (120 << 8))
#define XFORM_PIXEL_Y(x, y)	(xform_sa * (x) + xform_ca * (y) + (80 << 8))
static int32_t xform_sa, xform_ca;	/* for viewport bank/zoom */
static int xform_s;

static short vblcount;
static void *prev_iwram_top;

static int hit_frame;
static uint16_t color0;

static inline void xform_pixel(int *xp, int *yp);


struct screen *init_game_screen(void)
{
	return &gamescr;
}

static void setup_palette(void)
{
	int i;
	unsigned char *cmap = gba_colors ? color_gba_cmap : color_cmap;

	for(i=0; i<256; i++) {
		int r = cmap[i * 3];
		int g = cmap[i * 3 + 1];
		int b = cmap[i * 3 + 2];
		gba_bgpal[i] = RGB555(r, g, b);
	}

	color0 = gba_bgpal[0];
}

static int gamescr_start(void)
{
	int i, j, sidx;
	uint8_t *cptr;
	struct enemy *enemy;

	prev_iwram_top = iwram_sbrk(0);

	gba_setmode(4, DISPCNT_BG2 | DISPCNT_OBJ | DISPCNT_FB1);
	fillblock_16byte(gba_vram_lfb1, 0, 240 * 160 / 16);

	vblperf_setcolor(0);

	pos[0] = pos[1] = VOX_SZ << 15;
	angle = 0x8000;
	last_shot = -P_RATE - 1;

	vox_init(VOX_SZ, VOX_SZ, height_pixels, color_pixels);
	vox_proj(FOV, NEAR, FAR);
	pheight = vox_view(pos[0], pos[1], -40, angle);

	/* setup color image palette */
	setup_palette();

	spr_setup(16, 16, spr_game_pixels, gba_colors ? spr_game_gba_cmap : spr_game_cmap);
	wait_vblank();
	spr_clear();

	for(i=0; i<MAX_SPR; i++) {
		spr_oam_clear(oam, i);
	}

	sidx = 0;
	spr_oam(oam, sidx++, SPRID_CROSS, 120-8, 80-8, SPR_SZ16 | SPR_256COL);
	spr_oam(oam, sidx++, SPRID_UIMID, 0, 144, SPR_VRECT | SPR_256COL);
	spr_oam(oam, sidx++, SPRID_UIRIGHT, 48, 144, SPR_SZ16 | SPR_256COL);
	spr_oam(oam, sidx++, SPRID_UILEFT, 168, 144, SPR_SZ16 | SPR_256COL);
	spr_oam(oam, sidx++, SPRID_UITGT, 184, 144, SPR_SZ16 | SPR_256COL);
	spr_oam(oam, sidx++, SPRID_UISLASH, 216, 144, SPR_VRECT | SPR_256COL);
	dynspr_base = sidx;

	num_kills = total_enemies = 0;
	energy = 5;

	memset(enemies, 0, sizeof enemies);
	cptr = color_pixels;
	for(i=0; i<VOX_SZ; i++) {
		for(j=0; j<VOX_SZ; j++) {
			if(*cptr == 255) {
				/* player spawn point */
				pos[0] = j << 16;
				pos[1] = i << 16;

			} else if(*cptr >= CMAP_SPAWN0 && *cptr != 255) {
				/* enemy spawn point */
				int idx = *cptr - CMAP_SPAWN0;
				enemy = enemies + idx;
				if(enemy->anm) {
					panic(get_pc(), "double spawn %d at %d,%d (prev: %d,%d)", idx,
							j, i, enemy->vobj.x, enemy->vobj.y);
				}
				enemy->vobj.x = j;
				enemy->vobj.y = i;
				enemy->vobj.px = -1;
				enemy->anm = 0xff;
				enemy->hp = ENEMY_ENERGY;
				enemy->last_shot = -1;
				enemy->shot_frame = -1;
				if(++total_enemies >= MAX_ENEMIES) {
					goto endspawn;
				}
			}
			cptr++;
		}
	}
endspawn:
	/* check continuity */
	for(i=0; i<total_enemies; i++) {
		if(enemies[i].anm <= 0) {
			panic(get_pc(), "discontinuous enemy list");
		}
		enemies[i].anm = rand() & 7;
	}

	vox_objects((struct vox_object*)enemies, total_enemies, sizeof *enemies);

	energy = MAX_ENERGY;
	xform_sa = 0;
	xform_ca = 0x10000;
	xform_s = 0x100;

	gameover = 0;
	score = -1;
	total_time = 0;
	start_time = timer_msec;

	hitfrm = 0;

	vblcount = 0;
	nframes = 0;

	running = 1;
	return 0;
}

static void gamescr_stop(void)
{
	running = 0;

	iwram_brk(prev_iwram_top);

	wait_vblank();
	/* clear sprites */
	spr_clear();
	/* reset background rot/scale state */
	REG_BG2X = 0;
	REG_BG2Y = 0;
	REG_BG2PA = 0x100;
	REG_BG2PB = 0;
	REG_BG2PC = 0;
	REG_BG2PD = 0x100;

}

static void gamescr_frame(void)
{
	backbuf = ++nframes & 1;
	framebuf = vram[backbuf];

	vox_framebuf(240, 160, framebuf, horizon);

	if(update() == -1) {
		return;
	}
	draw();

	vblperf_end();
	wait_vblank();
	present(backbuf);

	/*
	if(!(nframes & 15)) {
		emuprint("vbl: %d", vblperf_count);
	}*/
#ifdef VBLBAR
	vblperf_begin();
#else
	vblperf_count = 0;
#endif
}

#define NS(x)	(SPRID_UINUM + ((x) << 1))
static int numspr[][2] = {
	{NS(0),NS(0)}, {NS(0),NS(1)}, {NS(0),NS(2)}, {NS(0),NS(3)}, {NS(0),NS(4)},
	{NS(0),NS(5)}, {NS(0),NS(6)}, {NS(0),NS(7)}, {NS(0),NS(8)}, {NS(0),NS(9)},
	{NS(1),NS(0)}, {NS(1),NS(1)}, {NS(1),NS(2)}, {NS(1),NS(3)}, {NS(1),NS(4)},
	{NS(1),NS(5)}, {NS(1),NS(6)}, {NS(1),NS(7)}, {NS(1),NS(8)}, {NS(1),NS(9)},
	{NS(2),NS(0)}, {NS(2),NS(1)}, {NS(2),NS(2)}, {NS(2),NS(3)}, {NS(2),NS(4)},
	{NS(2),NS(5)}, {NS(2),NS(6)}, {NS(2),NS(7)}, {NS(2),NS(8)}, {NS(2),NS(9)},
	{NS(3),NS(0)}, {NS(3),NS(1)}, {NS(3),NS(2)}, {NS(3),NS(3)}, {NS(3),NS(4)},
	{NS(3),NS(5)}, {NS(3),NS(6)}, {NS(3),NS(7)}, {NS(3),NS(8)}, {NS(3),NS(9)}
};

#define WALK_SPEED	0x40000
#define TURN_SPEED	0x200
#define ELEV_SPEED	8

#define MAX(a, b)	((a > (b) ? (a) : (b)))

static int update(void)
{
	int32_t fwd[2], right[2];
	int i, snum, ledspr;
	struct enemy *enemy;
	int did_strafe = 0;

	hit_frame = 0;

	update_keyb();

	if(KEYPRESS(BN_START)) {
		/* TODO pause menu */
		change_screen(find_screen("menu"));
		return -1;
	}

	if(gameover) {
		goto skip_game_logic;
	}

	if(keystate) {
		if(keystate & BN_LEFT) {
			angle += TURN_SPEED;
		}
		if(keystate & BN_RIGHT) {
			angle -= TURN_SPEED;
		}

		fwd[0] = -SIN(angle);
		fwd[1] = COS(angle);
		right[0] = fwd[1];
		right[1] = -fwd[0];

		if(keystate & BN_A) {
			pos[0] = (pos[0] + fwd[0]) & POS_MASK;
			pos[1] = (pos[1] + fwd[1]) & POS_MASK;
			if(pos[0] < 0) pos[0] += VOX_SZ << 16;
			if(pos[1] < 0) pos[1] += VOX_SZ << 16;
		}

		if((keystate & BN_B) && (timer_msec - last_shot >= P_RATE)) {
			last_shot = timer_msec;
			for(i=0; i<total_enemies; i++) {
				if(enemies[i].hp && enemies[i].vobj.px >= 0) {
					int dx = enemies[i].vobj.px - 120;
					int dy = enemies[i].vobj.py - 80;
					int rad = enemies[i].vobj.scale >> 5;

					if(rad < 1) rad = 1;

					if(abs(dx) < rad && abs(dy) < (rad << 1)) {
						enemies[i].shot_frame = -1;
						if(--enemies[i].hp <= 0) {
							if(++num_kills >= total_enemies) {
								victory();
							}
						}
						hit_px = enemies[i].vobj.px;
						hit_py = enemies[i].vobj.py;
						hitfrm = nframes;
						break;
					}
				}
			}
		}
		if(keystate & BN_UP) {
			if(horizon > 40) horizon -= ELEV_SPEED;
		}
		if(keystate & BN_DOWN) {
			if(horizon < 200 - ELEV_SPEED) horizon += ELEV_SPEED;
		}
		if(keystate & BN_RT) {
			pos[0] = (pos[0] + right[0]) & POS_MASK;
			pos[1] = (pos[1] + right[1]) & POS_MASK;
			if(pos[0] < 0) pos[0] += VOX_SZ << 16;
			if(pos[1] < 0) pos[1] += VOX_SZ << 16;
			did_strafe = 1;
		}
		if(keystate & BN_LT) {
			pos[0] = (pos[0] - right[0]) & POS_MASK;
			pos[1] = (pos[1] - right[1]) & POS_MASK;
			if(pos[0] < 0) pos[0] += VOX_SZ << 16;
			if(pos[1] < 0) pos[1] += VOX_SZ << 16;
			did_strafe = 1;
		}

		pheight = vox_view(pos[0], pos[1], -40, angle);
	}

	/* enemy logic */
	enemy = enemies;
	for(i=0; i<total_enemies; i++) {
		/* only consider enemies which are not dead */
		if(enemy->hp <= 0) {
			enemy++;
			continue;
		}

		if(enemy->shot_frame >= 0) {
			/* in the process of charging a shot */
			if(++enemy->shot_frame >= NUM_SHOT_FRAMES - 1) {
				/* only get hit if we can see the enemy and we didn't strafe */
				if(!did_strafe && enemy->vobj.px >= 0) {
					hit_frame = 1;
					if(--energy <= 0) {
						gameover = 1;
					}
				}
				enemy->shot_frame = -1;
			}
		} else if(enemy->vobj.px >= 0) {
			/* check rate of fire and start a shot if necessary */
			if(enemy->last_shot == -1) {
				enemy->last_shot = timer_msec;
			} else if(timer_msec - enemy->last_shot >= E_RATE) {
				enemy->last_shot = timer_msec;
				enemy->shot_frame = 0;
			}
		}
		enemy++;
	}

skip_game_logic:

	snum = 0;
	/* turrets number */
	spr_oam(oam, dynspr_base + snum++, numspr[num_kills][0], 200, 144, SPR_VRECT | SPR_256COL);
	spr_oam(oam, dynspr_base + snum++, numspr[num_kills][1], 208, 144, SPR_VRECT | SPR_256COL);
	spr_oam(oam, dynspr_base + snum++, numspr[total_enemies][0], 224, 144, SPR_VRECT | SPR_256COL);
	spr_oam(oam, dynspr_base + snum++, numspr[total_enemies][1], 232, 144, SPR_VRECT | SPR_256COL);
	/* energy bar */
	if(energy == MAX_ENERGY) {
		ledspr = SPRID_LEDBLU;
	} else {
		ledspr = energy > 2 ? SPRID_LEDGRN : SPRID_LEDRED;
	}
	for(i=0; i<5; i++) {
		spr_oam(oam, dynspr_base + snum++, i >= energy ? SPRID_LEDOFF : ledspr,
				8 + (i << 3), 144, SPR_VRECT | SPR_256COL);
	}
	/* blaster sprites */
	if(timer_msec - last_shot <= SHOT_TIME) {
		spr_oam(oam, dynspr_base + snum++, SPRID_LAS0, -8, 118, SPR_SZ32 | SPR_256COL);
		spr_oam(oam, dynspr_base + snum++, SPRID_LAS1, 22, 103, SPR_SZ32 | SPR_256COL);
		spr_oam(oam, dynspr_base + snum++, SPRID_LAS2, 54, 88, SPR_SZ32 | SPR_256COL);
		spr_oam(oam, dynspr_base + snum++, SPRID_LAS3, 86, 72, SPR_SZ32 | SPR_256COL);

		spr_oam(oam, dynspr_base + snum++, SPRID_LAS0, 240 + 8 - 32, 118, SPR_SZ32 | SPR_256COL | SPR_HFLIP);
		spr_oam(oam, dynspr_base + snum++, SPRID_LAS1, 240 - 22 - 32, 103, SPR_SZ32 | SPR_256COL | SPR_HFLIP);
		spr_oam(oam, dynspr_base + snum++, SPRID_LAS2, 240 - 54 - 32, 88, SPR_SZ32 | SPR_256COL | SPR_HFLIP);
		spr_oam(oam, dynspr_base + snum++, SPRID_LAS3, 240 - 86 - 32, 72, SPR_SZ32 | SPR_256COL | SPR_HFLIP);
	}
	/* hit sparks */
	if(nframes - hitfrm < 5) {
		int id = SPRID_SPARK0 + (nframes - hitfrm);
		spr_oam(oam, dynspr_base + snum++, id, hit_px - 16, hit_py - 16,
				SPR_DBLSZ | SPR_SZ16 | SPR_256COL | SPR_ROTSCL | SPR_ROTSCL_SEL(0));
	}
	/* enemy sprites */
	/*spr_oam(oam, dynspr_base + snum++, SPRID_ENEMY, 50, 50, SPR_VRECT | SPR_SZ64 | SPR_256COL);*/
	enemy = enemies;
	for(i=0; i<total_enemies; i++) {
		int sid, anm, px, py, yoffs;
		unsigned int flags;
		int16_t mat[4];
		int32_t sa, ca, scale;

		if(enemy->vobj.px >= 0) {
			flags = SPR_DBLSZ | SPR_256COL | SPR_ROTSCL | SPR_ROTSCL_SEL(0);
			if(enemy->hp > 0) {
				anm = (enemy->anm + (vblcount >> 3)) & 0xf;
				sid = SPRID_ENEMY0 + ((anm & 7) << 2);
				flags |= SPR_SZ32 | SPR_VRECT;
				yoffs = 32;
			} else {
				anm = 0;
				sid = SPRID_HUSK;
				flags |= SPR_SZ16;
				yoffs = 16;
			}

			px = enemy->vobj.px - 120;
			py = enemy->vobj.py - 80;
			xform_pixel(&px, &py);


			if(enemy->shot_frame >= 0) {
				if(enemy->shot_frame < SFRM_LVL1) {
					spr_oam(oam, dynspr_base + snum++, SPRID_SHOT0, px - 16, py - 16,
							SPR_DBLSZ | SPR_SZ16 | SPR_256COL | SPR_ROTSCL | SPR_ROTSCL_SEL(0));
				} else if(enemy->shot_frame < SFRM_LVL2) {
					spr_oam(oam, dynspr_base + snum++, SPRID_SHOT1, px - 16, py - 16,
							SPR_DBLSZ | SPR_SZ16 | SPR_256COL | SPR_ROTSCL | SPR_ROTSCL_SEL(0));
				} else {
					spr_oam(oam, dynspr_base + snum++, SPRID_SHOT2, px - 16, py - 16,
							SPR_DBLSZ | SPR_SZ16 | SPR_256COL | SPR_ROTSCL | SPR_ROTSCL_SEL(0));
				}
			}


			spr_oam(oam, dynspr_base + snum++, sid, px - 16, py - yoffs, flags);

			scale = enemy->vobj.scale;
			if(scale > 0x10000) scale = 0x10000;
			sa = xform_sa / scale;
			ca = xform_ca / scale;
			mat[0] = anm >= 8 ? -ca : ca;
			mat[1] = sa;
			mat[2] = -sa;
			mat[3] = ca;

			spr_transform(oam, 0, mat);
			enemy->vobj.px = -1;
		}
		enemy++;
	}
	for(i=snum; i<dynspr_count; i++) {
		spr_oam_clear(oam, dynspr_base + i);
	}

	mask(INTR_VBLANK);
	dynspr_count = snum;
	unmask(INTR_VBLANK);

	return 0;
}

static void draw(void)
{
	//dma_fill16(3, framebuf, 0, 240 * 160 / 2);
	fillblock_16byte(framebuf, 0, 240 * 160 / 16);

	if(hit_frame) {
		gba_bgpal[0] = 0x7fff;
	} else {
		gba_bgpal[0] = color0;
		vox_render();
	}
	//vox_sky_grad(COLOR_HORIZON, COLOR_ZENITH);
	//vox_sky_solid(COLOR_ZENITH);

	if(score >= 0 || energy <= 0) {
		int sec = total_time / 1000;

		fillblock_16byte(framebuf + 8 * 240 / 2, 199 | (199 << 8) | (199 << 16) | (199 << 24), 40 * 240 / 16);

		glyphfb = framebuf;
		glyphbg = 199;
		glyphcolor = 197;
		if(energy > 0) {
			dbg_drawstr(80, 10, "Victory!");
			glyphcolor = 200;
			dbg_drawstr(30, 20, "       Score: %d", score);
			dbg_drawstr(30, 28, "Completed in: %lum.%lus", sec / 60, sec % 60);
		} else {
			dbg_drawstr(80, 10, "Game Over!");
		}
		glyphcolor = 198;
		dbg_drawstr(85, 40, "Press start to exit");
	}
}

static void victory(void)
{
	int sec, time_bonus = 0;

	total_time = timer_msec - start_time;
	sec = total_time / 1000;

	if(sec < 60) {
		time_bonus = 1000 + (60 - sec) * 100;
	} else if(sec < 120) {
		time_bonus = 250 + (60 - (sec - 60)) * 10;
	} else if(sec < 300) {
		time_bonus = 100;
	}

	score = energy * 250 + time_bonus;

	/* TODO enter name */
	save_score("???", score, total_time, 0);
	save_scores();
}

static inline void xform_pixel(int *xp, int *yp)
{
	int32_t sa = xform_sa >> 8;
	int32_t ca = xform_ca >> 8;
	int x = *xp;
	int y = *yp;

	*xp = (ca * x - sa * y + (120 << 8)) >> 8;
	*yp = (sa * x + ca * y + (80 << 8)) >> 8;
}

#define MAXBANK		0x100

ARM_IWRAM
static void gamescr_vblank(void)
{
	static int bank, bankdir, theta;
	int32_t sa, ca;

	if(!running) return;

	vblcount++;

	/* TODO: pre-arrange sprite tiles in gba-native format, so that I can just
	 * DMA them from cartridge easily
	 */

	/*dma_copy32(3, (void*)(OAM_ADDR + dynspr_base * 8), oam + dynspr_base * 4, MAX_SPR * 2, 0);*/
	dma_copy32(3, (void*)OAM_ADDR, oam, MAX_SPR * 2, 0);

	if(gameover) return;

	theta = -(bank << 3);
	xform_sa = SIN(theta);
	xform_ca = COS(theta);
#if 0
	xform_s = 0x100000 / (MAXBANK + (abs(bank) >> 3));
	sa = (((xform_sa) >> 8) * xform_s) >> 12;
	ca = (((xform_ca) >> 8) * xform_s) >> 12;
#else
	xform_s = (MAXBANK + (abs(bank) >> 3));
	sa = xform_sa / xform_s;
	ca = xform_ca / xform_s;
#endif

	REG_BG2X = -ca * 120 - sa * 80 + (120 << 8);
	REG_BG2Y = sa * 120 - ca * 80 + (80 << 8);

	REG_BG2PA = ca;
	REG_BG2PB = sa;
	REG_BG2PC = -sa;
	REG_BG2PD = ca;

	if((keystate & (BN_LEFT | BN_RIGHT)) == 0) {
		if(bank) {
			bank -= bankdir << 4;
		}
	} else if(keystate & BN_LEFT) {
		bankdir = -1;
		if(bank > -MAXBANK) bank -= 16;
	} else if(keystate & BN_RIGHT) {
		bankdir = 1;
		if(bank < MAXBANK) bank += 16;
	}
}
