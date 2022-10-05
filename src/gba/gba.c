#include "gba.h"

void gba_setmode(int mode, unsigned int flags)
{
	REG_DISPCNT = mode | flags;
}

