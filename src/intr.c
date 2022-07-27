#include "intr.h"

#define MAX_INTR	14
static void (*intr_table[MAX_INTR])(void);

__attribute__ ((target("arm"), section(".iwram")))
static void intr_handler(void)
{
	int i;
	uint16_t iflags;

	iflags = REG_IF;

	for(i=0; i<MAX_INTR; i++) {
		if((iflags & (1 << i)) && intr_table[i]) {
			intr_table[i]();
		}
	}

	REG_IF = iflags;	/* ack intr */
}

void intr_init(void)
{
	INTR_VECTOR = (uint32_t)intr_handler;
}

void interrupt(int intr, void (*handler)(void))
{
	intr_table[intr] = handler;
}
