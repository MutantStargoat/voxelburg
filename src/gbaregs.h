#ifndef GBAREGS_H_
#define GBAREGS_H_

#include <stdint.h>

#define VRAM_START_ADDR		0x6000000
#define VRAM_BG_ADDR		VRAM_START_ADDR
#define VRAM_OBJ_ADDR		0x6010000
#define VRAM_LFB_OBJ_ADDR	0x6014000
#define VRAM_LFB_FB0_ADDR	VRAM_START_ADDR
#define VRAM_LFB_FB1_ADDR	0x600a000

/* address of character data block x (4 possible blocks, 16k each) */
#define VRAM_CHR_BLOCK_ADDR(x)	(VRAM_START_ADDR + ((x) << 14))
/* address of screen data block x (32 possible blocks, 2k each) */
#define VRAM_SCR_BLOCK_ADDR(x)	(VRAM_START_ADDR + ((x) << 11))

/* fields of a background tile in screen memory */
#define BGTILE_HFLIP	0x0400
#define BGTILE_VFLIP	0x0800
#define BGTILE_PAL(x)	((uint16_t)(x) << 12)

/* color palette ram addresses for backgrounds and sprites */
#define CRAM_BG_ADDR	0x5000000
#define CRAM_OBJ_ADDR	0x5000200

/* OAM table */
#define OAM_ADDR		0x7000000

/* interrupt handler */
#define INTR_VECTOR		(*(volatile uint32_t*)0x3007ffc)

/* battery backed RAM address */
#define SRAM_ADDR		0xe000000

/* I/O space */

#define REG_BASE		0x4000000
#define REG8(x)			(*(volatile uint8_t*)(REG_BASE + (x)))
#define REG16(x)		(*(volatile uint16_t*)(REG_BASE + (x)))
#define REG32(x)		(*(volatile uint32_t*)(REG_BASE + (x)))

/* ---- display registers ---- */
#define REG_DISPCNT		REG16(0x00)
#define REG_GREENSWAP	REG16(0x02)
#define REG_DISPSTAT	REG16(0x04)
#define REG_VCOUNT		REG16(0x06)
#define REG_BG0CNT		REG16(0x08)
#define REG_BG1CNT		REG16(0x0a)
#define REG_BG2CNT		REG16(0x0c)
#define REG_BG3CNT		REG16(0x0e)
/* scrolling registers */
#define REG_BG0HOFS		REG16(0x10)
#define REG_BG0VOFS		REG16(0x12)
#define REG_BG1HOFS		REG16(0x14)
#define REG_BG1VOFS		REG16(0x16)
#define REG_BG2HOFS		REG16(0x18)
#define REG_BG2VOFS		REG16(0x1a)
#define REG_BG3HOFS		REG16(0x1c)
#define REG_BG3VOFS		REG16(0x1e)
/* BG rotation and scaling registers */
#define REG_BG2PA		REG16(0x20)
#define REG_BG2PB		REG16(0x22)
#define REG_BG2PC		REG16(0x24)
#define REG_BG2PD		REG16(0x26)
#define REG_BG2X		REG32(0x28)
#define REG_BG2Y		REG32(0x2c)
#define REG_BG3PA		REG16(0x30)
#define REG_BG3PB		REG16(0x32)
#define REG_BG3PC		REG16(0x34)
#define REG_BG3PD		REG16(0x36)
#define REG_BG3X		REG32(0x38)
#define REG_BG3Y		REG32(0x3c)
/* window registers */
#define REG_WIN0H		REG16(0x40)
#define REG_WIN1H		REG16(0x42)
#define REG_WIN0V		REG16(0x44)
#define REG_WIN1V		REG16(0x46)
#define REG_WININ		REG16(0x48)
#define REG_WINOUT		REG16(0x4a)

#define REG_MOSAIC		REG16(0x4c)
/* color effects */
#define REG_BLDCNT		REG16(0x50)
#define REG_BLDALPHA	REG16(0x52)
#define REG_BLDY		REG16(0x54)

/* ---- sound registers ---- */
#define REG_SOUND1CNT_L	REG16(0x60)
#define REG_SOUND1CNT_H	REG16(0x62)
#define REG_SOUND1CNT_X	REG16(0x64)
#define REG_SOUND2CNT_L	REG16(0x68)
#define REG_SOUND2CNT_H	REG16(0x6c)
#define REG_SOUND3CNT_L	REG16(0x70)
#define REG_SOUND3CNT_H	REG16(0x72)
#define REG_SOUND3CNT_X	REG16(0x74)
#define REG_SOUND4CNT_L	REG16(0x78)
#define REG_SOUND4CNT_H	REG16(0x7c)
#define REG_SOUNDCNT_L	REG16(0x80)
#define REG_SOUNDCNT_H	REG16(0x82)
#define REG_SOUNDCNT_X	REG16(0x84)
#define REG_SOUNDBIAS	REG16(0x88)
#define WAVE_RAM_PTR	((unsigned char*)(REG_BASE + 0x90))
#define REG_FIFO_A		REG32(0xa0)
#define REG_FIFO_B		REG32(0xa4)
#define FIFO_A_PTR		((unsigned char*)(REG_BASE + 0xa0))
#define FIFO_B_PTR		((unsigned char*)(REG_BASE + 0xa4))

/* ---- DMA registers ---- */
#define REG_DMA0SAD		REG32(0xb0)
#define REG_DMA0DAD		REG32(0xb4)
#define REG_DMA0CNT		REG32(0xb8)
#define REG_DMA0CNT_L	REG16(0xb8)
#define REG_DMA0CNT_H	REG16(0xba)
#define REG_DMA1SAD		REG32(0xbc)
#define REG_DMA1DAD		REG32(0xc0)
#define REG_DMA1CNT		REG32(0xc4)
#define REG_DMA1CNT_L	REG16(0xc4)
#define REG_DMA1CNT_H	REG16(0xc6)
#define REG_DMA2SAD		REG32(0xc8)
#define REG_DMA2DAD		REG32(0xcc)
#define REG_DMA2CNT		REG32(0xd0)
#define REG_DMA2CNT_L	REG16(0xd0)
#define REG_DMA2CNT_H	REG16(0xd2)
#define REG_DMA3SAD		REG32(0xd4)
#define REG_DMA3DAD		REG32(0xd8)
#define REG_DMA3CNT		REG32(0xdc)
#define REG_DMA3CNT_L	REG16(0xdc)
#define REG_DMA3CNT_H	REG16(0xde)

/* ---- timer registers ---- */
#define REG_TM0CNT_L	REG16(0x100)
#define REG_TM0CNT_H	REG16(0x102)
#define REG_TM1CNT_L	REG16(0x104)
#define REG_TM1CNT_H	REG16(0x106)
#define REG_TM2CNT_L	REG16(0x108)
#define REG_TM2CNT_H	REG16(0x10a)
#define REG_TM3CNT_L	REG16(0x10c)
#define REG_TM3CNT_H	REG16(0x10e)

#define REG_TMCNT_L(x)	REG16(0x100 + ((x) << 2))
#define REG_TMCNT_H(x)	REG16(0x102 + ((x) << 2))

/* ---- communication registers (serial/joybus/gpio) ---- */
#define REG_SIODATA32	REG32(0x120)
#define REG_SIOMULTI0	REG16(0x120)
#define REG_SIOMULTI1	REG16(0x122)
#define REG_SIOMULTI2	REG16(0x124)
#define REG_SIOMULTI3	REG16(0x126)
#define REG_SIOCNT		REG16(0x128)
#define REG_SIOMLT_SEND	REG16(0x12a)
#define REG_SIODATA8	REG16(0x12a)
#define REG_RCNT		REG16(0x134)
#define REG_JOYCNT		REG16(0x140)
#define REG_JOY_RECV	REG32(0x150)
#define REG_JOY_TRANS	REG32(0x154)
#define REG_JOYSTAT		REG16(0x158)

/* ---- keypad registers ---- */
#define REG_KEYINPUT	REG16(0x130)
#define REG_KEYCNT		REG16(0x132)

/* ---- interrupts ---- */
#define REG_IE			REG16(0x200)
#define REG_IF			REG16(0x202)
#define REG_WAITCNT		REG16(0x204)
#define REG_IME			REG16(0x208)

#define REG_POSTFLG		REG8(0x300)
#define REG_HALTCNT		REG8(0x301)
#define REG_INTMEMCNT	REG32(0x800)

/* REG_DISPSTAT bits */
#define DISPSTAT_VBLANK			0x01
#define DISPSTAT_HBLANK			0x02
#define DISPSTAT_VMATCH			0x04
#define DISPSTAT_IEN_VBLANK		0x08
#define DISPSTAT_IEN_HBLANK		0x10
#define DISPSTAT_IEN_VMATCH		0x20
#define DISPSTAT_VCOUNT(x)		((uint16_t)(x) << 8)

/* REG_DISPCNT bits */
#define DISPCNT_MODE(x)			(x)
#define DISPCNT_FB1				0x0010
#define DISPCNT_HBLANK_OBJPROC	0x0020
#define DISPCNT_OBJMAP_1D		0x0040
#define DISPCNT_FORCE_BLANK		0x0080
#define DISPCNT_BG0				0x0100
#define DISPCNT_BG1				0x0200
#define DISPCNT_BG2				0x0400
#define DISPCNT_BG3				0x0800
#define DISPCNT_OBJ				0x1000
#define DISPCNT_WIN0			0x2000
#define DISPCNT_WIN1			0x4000
#define DISPCNT_WINOBJ			0x8000

/* REG_BGXCNT bits */
#define BGCNT_PRIO(x)			((uint16_t)(x))
#define BGCNT_CHR_BASE(x)		((uint16_t)(x) << 2)
#define BGCNT_MOSAIC			0x0040
#define BGCNT_256COL			0x0080
#define BGCNT_SCR_BASE(x)		((uint16_t)(x) << 8)
#define BGCNT_WRAP				0x2000

#define BGCNT_SZ(x)				((uint16_t)(x) << 14)
#define BGCNT_SZ_TX_256X256		BGCNT_SZ(0)
#define BGCNT_SZ_RS_128X128		BGCNT_SZ(0)
#define BGCNT_SZ_TX_512X256		BGCNT_SZ(1)
#define BGCNT_SZ_RS_256X256		BGCNT_SZ(1)
#define BGCNT_SZ_TX_256X512		BGCNT_SZ(2)
#define BGCNT_SZ_RS_512X512		BGCNT_SZ(2)
#define BGCNT_SZ_TX_512X512		BGCNT_SZ(3)
#define BGCNT_SZ_RS_1024X1024	BGCNT_SZ(3)

/* REG_BLDCNT bits */
#define BLDCNT_A_BG0		0x0001
#define BLDCNT_A_BG1		0x0002
#define BLDCNT_A_BG2		0x0004
#define BLDCNT_A_BG3		0x0008
#define BLDCNT_A_OBJ		0x0010
#define BLDCNT_A_BACKDROP	0x0020
#define BLDCNT_B_BG0		0x0100
#define BLDCNT_B_BG1		0x0200
#define BLDCNT_B_BG2		0x0400
#define BLDCNT_B_BG3		0x0800
#define BLDCNT_B_OBJ		0x1000
#define BLDCNT_B_BACKDROP	0x2000

#define BLDCNT_ALPHA		0x0040
#define BLDCNT_BRIGHTEN		0x0080
#define BLDCNT_DARKEN		0x00c0

/* REG_IF bits */
#define IF_VBLANK		0x0001
#define IF_HBLANK		0x0002
#define IF_VMATCH		0x0004
#define IF_TIMER0		0x0008
#define IF_TIMER1		0x0010
#define IF_TIMER2		0x0020
#define IF_TIMER3		0x0040
#define IF_COMM			0x0080
#define IF_DMA0			0x0100
#define IF_DMA1			0x0200
#define IF_DMA2			0x0400
#define IF_DMA3			0x0800
#define IF_KEY			0x1000
#define IF_GPAK			0x2000

/* REG_TMXCNT bits */
#define TMCNT_PRESCL_CLK1		0
#define TMCNT_PRESCL_CLK64		1
#define TMCNT_PRESCL_CLK256		2
#define TMCNT_PRESCL_CLK1024	3

#define TMCNT_CASCADE			0x04
#define TMCNT_IE				0x40
#define TMCNT_EN				0x80

/* REG_KEY* bits */
#define KEY_A			0x0001
#define KEY_B			0x0002
#define KEY_SELECT		0x0004
#define KEY_START		0x0008
#define KEY_RIGHT		0x0010
#define KEY_LEFT		0x0020
#define KEY_UP			0x0040
#define KEY_DOWN		0x0080
#define KEY_RT			0x0100
#define KEY_LT			0x0200

#define KEYCNT_IE		0x4000
#define KEYCNT_IAND		0x8000

/* REG_SOUNDCNT_L bits */
#define SCNT_SS_LVOL(x)		((x) & 7)
#define SCNT_SS_RVOL(x)		(((x) & 7) << 4)
#define SCNT_SS_VOL(x)		(SCNT_SS_LVOL(x) | SCNT_SS_RVOL(x))
#define SCNT_SS1_EN_R		0x0100
#define SCNT_SS2_EN_R		0x0200
#define SCNT_SS3_EN_R		0x0400
#define SCNT_SS4_EN_R		0x0800
#define SCNT_SS_EN_R(x)		(SCNT_SS1_EN_R << (x))
#define SCNT_SS1_EN_L		0x1000
#define SCNT_SS2_EN_L		0x2000
#define SCNT_SS3_EN_L		0x4000
#define SCNT_SS4_EN_L		0x8000
#define SCNT_SS_EN_L(x)		(SCNT_SS1_EN_L << (x))
#define SCNT_SS1_EN			(SCNT_SS1_EN_R | SCNT_SS1_EN_L)
#define SCNT_SS2_EN			(SCNT_SS2_EN_R | SCNT_SS2_EN_L)
#define SCNT_SS3_EN			(SCNT_SS3_EN_R | SCNT_SS3_EN_L)
#define SCNT_SS4_EN			(SCNT_SS4_EN_R | SCNT_SS4_EN_L)
#define SCNT_SS_EN(x)		(SCNT_SS_EN_L(x) | SCNT_SS_EN_R(x))

#define SCNT_SS1		0
#define SCNT_SS2		1
#define SCNT_SS3		2
#define SCNT_SS4		3

/* REG_SOUNDCNT_X bits */
#define SCNT_MASTER_EN		0x0080

/* REG_SOUNDCNT_H bits */
#define SCNT_SS_VOL_QRT		0x0000
#define SCNT_SS_VOL_HALF	0x0001
#define SCNT_SS_VOL_FULL	0x0002
#define SCNT_DSA_VOL_HALF	0
#define SCNT_DSA_VOL_FULL	0x0004
#define SCNT_DSB_VOL_HALF	0
#define SCNT_DSB_VOL_FULL	0x0008
#define SCNT_DSA_EN_R		0x0100
#define SCNT_DSA_EN_L		0x0200
#define SCNT_DSA_TIMER0		0
#define SCNT_DSA_TIMER1		0x0400
#define SCNT_DSA_CLRFIFO	0x0800
#define SCNT_DSB_EN_R		0x1000
#define SCNT_DSB_EN_L		0x2000
#define SCNT_DSB_TIMER0		0
#define SCNT_DSB_TIMER1		0x4000
#define SCNT_DSB_CLRFIFO	0x8000

/* REG_DMAxCNT_H bits */
#define DMACNTH_DST_INC		0
#define DMACNTH_DST_DEC		0x0020
#define DMACNTH_DST_FIXED	0x0040
#define DMACNTH_INC_RELOAD	0x0060
#define DMACNTH_SRC_INC		0
#define DMACNTH_SRC_DEC		0x0080
#define DMACNTH_SRC_FIXED	0x0100
#define DMACNTH_REPEAT		0x0200
#define DMACNTH_16BIT		0
#define DMACNTH_32BIT		0x0400
#define DMACNTH_VBLANK		0x1000
#define DMACNTH_HBLANK		0x2000
#define DMACNTH_SOUND		0x3000
#define DMACNTH_IEN			0x4000
#define DMACNTH_EN			0x8000

#define DMACNT_DST_INC		0
#define DMACNT_DST_DEC		0x00200000
#define DMACNT_DST_FIXED	0x00400000
#define DMACNT_INC_RELOAD	0x00600000
#define DMACNT_SRC_INC		0
#define DMACNT_SRC_DEC		0x00800000
#define DMACNT_SRC_FIXED	0x01000000
#define DMACNT_REPEAT		0x02000000
#define DMACNT_16BIT		0
#define DMACNT_32BIT		0x04000000
#define DMACNT_VBLANK		0x10000000
#define DMACNT_HBLANK		0x20000000
#define DMACNT_SOUND		0x30000000
#define DMACNT_IEN			0x40000000
#define DMACNT_EN			0x80000000

/* REG_WAITCNT bits */
#define WAITCNT_ROM_4_2		0x0000
#define WAITCNT_ROM_3_2		0x0004
#define WAITCNT_ROM_2_2		0x0008
#define WAITCNT_ROM_8_2		0x000c
#define WAITCNT_ROM_4_1		0x0010
#define WAITCNT_ROM_3_1		0x0014
#define WAITCNT_ROM_2_1		0x0018
#define WAITCNT_ROM_8_1		0x001c
#define WAITCNT_PREFETCH	0x4000

#endif	/* GBAREGS_H_ */
