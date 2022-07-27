#include "intr.h"
#include "timer.h"

#define F_CLK	16780000
/* clock is 16.78MHz
 * - no prescale: 59.595ns
 * - prescale 64: 3.814us
 * - prescale 256: 15.256us
 * - prescale 1024: 61.025us
 */

static void timer_intr(void);

void init_timer(int tm, unsigned long rate_hz, void (*intr)(void))
{
	static const unsigned long clk[] = {F_CLK, F_CLK / 64, F_CLK / 256, F_CLK / 1024};
	unsigned long count;
	int pscl = 0;

	do {
		count = clk[pscl] / rate_hz;
	} while(count >= 65536 && ++pscl < 4);

	if(pscl >= 4) return;	/* impossible rate */

	REG_TMCNT_H(tm) = 0;
	REG_TMCNT_L(tm) = 65536 - count;
	if(intr) {
		interrupt(INTR_TIMER0 + tm, intr);
		unmask(INTR_TIMER0 + tm);
		REG_TMCNT_H(tm) = TMCNT_IE;
	}
	REG_TMCNT_H(tm) |= TMCNT_EN | pscl;
}

void reset_msec_timer(void)
{
	REG_TM0CNT_H &= ~TMCNT_EN;
	interrupt(INTR_TIMER0, timer_intr);
	timer_msec = 0;
	REG_TM0CNT_L = 65535 - 16779;
	REG_TM0CNT_H |= TMCNT_IE | TMCNT_EN;
	unmask(INTR_TIMER0);
}

void delay(unsigned long ms)
{
	unsigned long end = timer_msec + ms;
	while(timer_msec < end);
}

static void timer_intr(void)
{
	timer_msec++;
}
