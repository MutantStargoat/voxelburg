#ifndef GBA_H_
#define GBA_H_

#include <stdint.h>
#include "gbaregs.h"
#include "intr.h"

#ifdef BUILD_GBA
#define gba_bgpal	((uint16_t*)CRAM_BG_ADDR)
#define gba_objpal	((uint16_t*)CRAM_OBJ_ADDR)

#define gba_vram		((uint16_t*)VRAM_START_ADDR)
#define gba_vram_lfb0	((uint16_t*)VRAM_LFB_FB0_ADDR)
#define gba_vram_lfb1	((uint16_t*)VRAM_LFB_FB1_ADDR)

#else
extern uint16_t gba_bgpal[256], gba_objpal[256];

extern uint16_t gba_vram[96 * 1024];
#define gba_vram_lfb0	gba_vram
#define gba_vram_lfb1	(uint16_t*)((char*)gba_vram + 0xa000)
#endif

void gba_setmode(int mode, unsigned int flags);

#endif	/* GBA_H_ */
