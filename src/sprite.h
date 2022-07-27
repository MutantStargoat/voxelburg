#ifndef SPRITE_H_
#define SPRITE_H_

#include <stdint.h>

enum {
	SPR_ROTSCL	= 0x0100,
	SPR_DBLSZ	= 0x0200,
	SPR_BLEND	= 0x0400,
	SPR_OBJWIN	= 0x0800,
	SPR_MOSAIC	= 0x1000,
	SPR_256COL	= 0x2000,
	SPR_HRECT	= 0x4000,
	SPR_VRECT	= 0x8000,

	SPR_HFLIP	= 0x100000,
	SPR_VFLIP	= 0x200000,
	SPR_SZ16	= 0x400000,
	SPR_SZ32	= 0x800000,
	SPR_SZ64	= 0xc00000
};
#define SPR_SZ8		0
#define SPR_ROTSCL_SEL(x)	((unsigned int)(x) << 17)
#define SPR_PRIO(x)			((unsigned int)(x) & 3)


struct hwsprite {
	short id;
	short width, height;
	short x, y;
	unsigned int flags;
};

struct sprite {
	short x, y;
	struct hwsprite hwspr[8];
	short num_hwspr;
};

void spr_setup(int xtiles, int ytiles, unsigned char *pixels, unsigned char *cmap);
void spr_clear(void);

#define spr_oam_clear(oam, idx) spr_oam(oam, idx, 0, 0, 160, 0)
void spr_oam(uint16_t *oam, int idx, int spr, int x, int y, unsigned int flags);
void spr_spr_oam(uint16_t *oam, int idx, struct sprite *spr);

/* idx is the rotation/scale parameter index (0-31), not the sprite index */
void spr_transform(uint16_t *oam, int idx, int16_t *mat);


#endif	/* SPRITE_H_ */
