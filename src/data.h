#ifndef DATA_H_
#define DATA_H_

#include <stdint.h>
/*#include "data/snd.h"*/

#define CONV_RGB24_RGB15(r, g, b) \
	(((r) >> 3) | (((uint16_t)(g) & 0xf8) << 2) | (((uint16_t)(b) & 0xf8) << 7))

#define VOX_SZ	512

enum {
	SPRID_BASE		= 512,
	SPRID_UILEFT	= SPRID_BASE + 0,
	SPRID_UIMID		= SPRID_BASE + 4,
	SPRID_UIRIGHT	= SPRID_BASE + 8,
	SPRID_UINUM		= SPRID_BASE + 12,
	SPRID_UISLASH	= SPRID_BASE + 94,
	SPRID_CROSS		= SPRID_BASE + 64,
	SPRID_UITGT		= SPRID_BASE + 68,
	SPRID_LEDOFF	= SPRID_BASE + 72,
	SPRID_LEDRED	= SPRID_BASE + 74,
	SPRID_LEDBLU	= SPRID_BASE + 76,
	SPRID_LEDGRN	= SPRID_BASE + 78
};

extern unsigned char color_pixels[];
extern unsigned char color_cmap[];
extern unsigned char height_pixels[];

extern unsigned char spr_game_pixels[];
extern unsigned char spr_game_cmap[];

#endif	/* DATA_H_ */
