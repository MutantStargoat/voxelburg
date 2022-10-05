#ifndef TIMER_H_
#define TIMER_H_

#include "gbaregs.h"

#define enable_timer(x) \
	do { REG_TMCNT_H(x) |= TMCNT_EN; } while(0)

#define disable_timer(x) \
	do { REG_TMCNT_H(x) &= ~TMCNT_EN; } while(0)

volatile unsigned long timer_msec;

void init_timer(int tm, unsigned long rate_hz, void (*intr)(void));

void reset_msec_timer(void);

void delay(unsigned long ms);

#ifdef __thumb__
#define udelay(x)  asm volatile ( \
	"0: sub %0, %0, #1\n\t" \
	"bne 0b\n\t" \
	:: "r"(x) : "cc")
#else
#define udelay(x)  asm volatile ( \
	"0: subs %0, %0, #1\n\t" \
	"bne 0b\n\t" \
	:: "r"(x) : "cc")
#endif

#endif	/* TIMER_H_ */
