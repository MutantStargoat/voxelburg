#include <string.h>
#include "intr.h"

static void (*intrfunc[14])(void);
static unsigned int intrmask;

#define IE	0x8000

void intr_init(void)
{
	memset(intrfunc, 0, sizeof intrfunc);
}

void interrupt(int intr, void (*handler)(void))
{
	intrfunc[intr] = handler;
}

void intr_enable(void)
{
	intrmask |= IE;
}

void intr_disable(void)
{
	intrmask &= ~IE;
}

void mask(int intr)
{
	intrmask &= ~(1 << intr);
}

void unmask(int intr)
{
	intrmask |= 1 << intr;
}
