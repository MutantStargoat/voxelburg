#ifndef DATA_H_
#define DATA_H_

#include <stdint.h>
/*#include "data/snd.h"*/

#define CONV_RGB24_RGB15(r, g, b) \
	(((r) >> 3) | (((uint16_t)(g) & 0xf8) << 2) | (((uint16_t)(b) & 0xf8) << 7))

#define VOX_SZ	512

#define SPRID(x, y)		(SPRID_BASE + ((y) * 4) + (x) / 4)

enum {
	SPRID_BASE		= 512,
	SPRID_UILEFT	= SPRID(0, 0),
	SPRID_UIMID		= SPRID(16, 0),
	SPRID_UIRIGHT	= SPRID(32, 0),
	SPRID_UINUM		= SPRID(48, 0),
	SPRID_UISLASH	= SPRID(120, 16),
	SPRID_CROSS		= SPRID(0, 16),
	SPRID_UITGT		= SPRID(16, 16),
	SPRID_LEDOFF	= SPRID(32, 16),
	SPRID_LEDRED	= SPRID(40, 16),
	SPRID_LEDBLU	= SPRID(48, 16),
	SPRID_LEDGRN	= SPRID(56, 16),
	/*SPRID_ENEMY		= SPRID(0, 64)*/
	SPRID_ENEMY0	= SPRID(0, 32),
	SPRID_HUSK		= SPRID(112, 64)
};

extern unsigned char color_pixels[];
extern unsigned char color_cmap[];
extern unsigned char height_pixels[];

extern unsigned char spr_game_pixels[];
extern unsigned char spr_game_cmap[];

#endif	/* DATA_H_ */
