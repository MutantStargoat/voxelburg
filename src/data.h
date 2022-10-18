#ifndef DATA_H_
#define DATA_H_

#include <stdint.h>
/*#include "data/snd.h"*/

#define CONV_RGB24_RGB15(r, g, b) \
	(((r) >> 3) | (((uint16_t)(g) & 0xf8) << 2) | (((uint16_t)(b) & 0xf8) << 7))

#define VOX_SZ	256

extern unsigned char color_pixels[];
extern unsigned char color_cmap[];
extern unsigned char height_pixels[];

#endif	/* DATA_H_ */
