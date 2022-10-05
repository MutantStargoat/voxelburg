#include "dma.h"

/* DMA Options */
#define DMA_ENABLE				0x80000000
#define DMA_INT_ENABLE			0x40000000
#define DMA_TIMING_IMMED		0x00000000
#define DMA_TIMING_VBLANK		0x10000000
#define DMA_TIMING_HBLANK		0x20000000
#define DMA_TIMING_DISPSYNC		0x30000000
#define DMA_16					0x00000000
#define DMA_32					0x04000000
#define DMA_REPEAT				0x02000000
#define DMA_SRC_INC				0x00000000
#define DMA_SRC_DEC				0x00800000
#define DMA_SRC_FIX				0x01000000
#define DMA_DST_INC				0x00000000
#define DMA_DST_DEC				0x00200000
#define DMA_DST_FIX1			0x00400000
#define DMA_DST_RELOAD			0x00600000

/* DMA Register Parts */
#define DMA_SRC		0
#define DMA_DST		1
#define DMA_CTRL	2

static volatile uint32_t *reg_dma[4] = {(void*)0x040000b0, (void*)0x040000bc, (void*)0x040000c8, (void*)0x040000d4 };

/* --- perform a copy of words or halfwords using DMA --- */

void dma_copy32(int channel, void *dst, void *src, int words, unsigned int flags)
{
	reg_dma[channel][DMA_SRC] = (uint32_t)src;
	reg_dma[channel][DMA_DST] = (uint32_t)dst;
	reg_dma[channel][DMA_CTRL] = words | flags | DMA_32 | DMA_ENABLE;
}

void dma_copy16(int channel, void *dst, void *src, int halfwords, unsigned int flags)
{
	reg_dma[channel][DMA_SRC] = (uint32_t)src;
	reg_dma[channel][DMA_DST] = (uint32_t)dst;
	reg_dma[channel][DMA_CTRL] = halfwords | flags | DMA_16 | DMA_ENABLE;
}

/* --- fill a buffer with an ammount of words and halfwords using DMA --- */

static uint32_t fill[4];

void dma_fill32(int channel, void *dst, uint32_t val, int words)
{
	fill[channel] = val;
	reg_dma[channel][DMA_SRC] = (uint32_t)(fill + channel);
	reg_dma[channel][DMA_DST] = (uint32_t)dst;
	reg_dma[channel][DMA_CTRL] = words | DMA_SRC_FIX | DMA_TIMING_IMMED | DMA_32 | DMA_ENABLE;
}

void dma_fill16(int channel, void *dst, uint16_t val, int halfwords)
{
	fill[channel] = val;
	reg_dma[channel][DMA_SRC] = (uint32_t)(fill + channel);
	reg_dma[channel][DMA_DST] = (uint32_t)dst;
	reg_dma[channel][DMA_CTRL] = halfwords | DMA_SRC_FIX | DMA_TIMING_IMMED | DMA_16 | DMA_ENABLE;
}
