#include "input.h"
#include "gbaregs.h"
#include "util.h"

/*
#define NUM_KEYS	10

static int rep_start, rep_rep;
static unsigned long first_press[16], last_press[16];
static uint16_t repmask;

void key_repeat(int start, int rep, uint16_t mask)
{
	rep_start = start;
	rep_rep = rep;
	repmask = mask;
}
*/

void update_keyb(void)
{
	static uint16_t prevstate;
	/*
	int i;
	unsigned long msec = timer_msec;
	*/

	//keystate = (~REG_KEYINPUT & 0x3ff);
	keydelta = keystate ^ prevstate;
	prevstate = keystate;

	/*
	for(i=0; i<NUM_KEYS; i++) {
		uint16_t bit = 1 << i;
		if(!(bit & repmask)) {
			continue;
		}

		if(keystate & bit) {
			if(keydelta & bit) {
				first_press[i] = msec;
			} else {
				if(msec - first_press[i] >= rep_start && msec - last_press[i] >= rep_rep) {
					keydelta |= bit;
					last_press[i] = msec;
				}
			}
		}
	}
	*/
}

