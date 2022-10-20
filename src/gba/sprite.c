#include "sprite.h"
#include "gbaregs.h"


void spr_setup(int xtiles, int ytiles, unsigned char *pixels, unsigned char *cmap)
{
	int i, j, num_tiles;
	uint16_t *cptr, *src, *dst;

	num_tiles = xtiles * ytiles;
	dst = (uint16_t*)VRAM_LFB_OBJ_ADDR;
	src = (uint16_t*)pixels;
	for(i=0; i<num_tiles; i++) {
		/* copy every row of tile i (8x8) */
		for(j=0; j<8; j++) {
			*dst++ = src[j * 64];
			*dst++ = src[j * 64 + 1];
			*dst++ = src[j * 64 + 2];
			*dst++ = src[j * 64 + 3];
		}
		src += 4;

		if((i & 15) == 15) {
			src += 7 * 64;	/* skip to the next row of tiles (skip 7 rows of pixels) */
		}
	}

	cptr = (uint16_t*)CRAM_OBJ_ADDR;
	for(i=0; i<128; i++) {
		unsigned char r = *cmap++ >> 3;
		unsigned char g = *cmap++ >> 3;
		unsigned char b = *cmap++ >> 3;
		*cptr++ = r | ((uint16_t)g << 5) | ((uint16_t)b << 10);
	}
}

void spr_clear(void)
{
	int i;

	for(i=0; i<128; i++) {
		spr_oam_clear(0, i);
	}
}

void spr_oam(uint16_t *oam, int idx, int spr, int x, int y, unsigned int flags)
{
	if(!oam) oam = (uint16_t*)OAM_ADDR;

	oam += idx << 2;

	oam[0] = (y & 0xff) | (flags & 0xff00);
	oam[1] = (x & 0x1ff) | ((flags >> 8) & 0xfe00);
	oam[2] = (spr & 0x3ff) | ((flags & 3) << 10);
}

void spr_spr_oam(uint16_t *oam, int idx, struct sprite *spr)
{
	int i;
	struct hwsprite *s;

	s = spr->hwspr;
	for(i=0; i<spr->num_hwspr; i++) {
		spr_oam(oam, idx, s->id, spr->x + s->x, spr->y + s->y, s->flags);
		s++;
	}
}

void spr_transform(uint16_t *oam, int idx, int16_t *mat)
{
	if(!oam) oam = (uint16_t*)OAM_ADDR;

	oam += (idx << 4) + 3;

	oam[0] = *mat++;
	oam[4] = *mat++;
	oam[8] = *mat++;
	oam[12] = *mat;
}
