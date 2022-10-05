#ifndef DMA_H_
#define DMA_H_

#include <stdint.h>

void dma_copy32(int channel, void *dst, void *src, int words, unsigned int flags);
void dma_copy16(int channel, void *dst, void *src, int halfwords, unsigned int flags);

void dma_fill32(int channel, void *dst, uint32_t val, int words);
void dma_fill16(int channel, void *dst, uint16_t val, int halfwords);

#endif	/* DMA_H_ */
