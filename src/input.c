#include "input.h"
#include "gbaregs.h"
#include "intr.h"

static void keyintr(void);

static uint16_t bnstate;

void select_input(uint16_t bmask)
{
	bnstate = 0;

	mask(INTR_KEY);
	if(bmask) {
		REG_KEYCNT = bmask | KEYCNT_IE;
		interrupt(INTR_KEY, keyintr);
		unmask(INTR_KEY);
	} else {
		REG_KEYCNT = 0;
		interrupt(INTR_KEY, 0);
	}
}

uint16_t get_input(void)
{
	uint16_t s;

	mask(INTR_KEY);
	s = bnstate;
	bnstate = 0;
	unmask(INTR_KEY);

	return s;
}

static void keyintr(void)
{
	bnstate |= ~REG_KEYINPUT;
}
